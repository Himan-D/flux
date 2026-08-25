#pragma once

#include <cmath>
#include <vector>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <chrono>

namespace flux::pricing {

struct AsianPricerInput {
    bool is_call;
    double strike;
    double risk_free_rate;
    double time_to_maturity;         // Time to final fixing date (years)
    std::vector<double> forward_strip; // Forwards F(t_j) for remaining fixings
    std::vector<double> time_points;   // Observation times t_j in years
    std::vector<double> volatilities;  // Implied vol for each tenor
    std::vector<double> realized_fixings; // Historical realized prices
    size_t total_fixings_count;
};

struct PricingResult {
    double price;
    double delta;
    double gamma;
    double vega;
    double theta;
    uint64_t latency_nanos;
};

class TurnbullWakemanAsianKernel {
public:
    static inline double norm_cdf(double x) noexcept {
        return 0.5 * std::erfc(-x * M_SQRT1_2);
    }

    static inline double norm_pdf(double x) noexcept {
        static constexpr double INV_SQRT_2PI = 0.3989422804014327;
        return INV_SQRT_2PI * std::exp(-0.5 * x * x);
    }

    static PricingResult price(const AsianPricerInput& input) {
        auto start_time = std::chrono::high_resolution_clock::now();

        const size_t m = input.realized_fixings.size();
        const size_t N = input.total_fixings_count;
        const size_t k = input.forward_strip.size();

        if (m + k != N || k == 0) {
            throw std::invalid_argument("Mismatch between fixings count and forward strip.");
        }

        // 1. Calculate Realized Component
        double realized_sum = std::accumulate(input.realized_fixings.begin(), input.realized_fixings.end(), 0.0);
        double remaining_weight = static_cast<double>(k) / static_cast<double>(N);
        double adjusted_strike = input.strike - (m > 0 ? (realized_sum / N) : 0.0);

        // 2. Compute 1st and 2nd Moments of Future Average
        double M1 = 0.0;
        for (size_t i = 0; i < k; ++i) {
            M1 += input.forward_strip[i];
        }
        M1 /= static_cast<double>(k);

        double M2 = 0.0;
        for (size_t i = 0; i < k; ++i) {
            double ti = input.time_points[i];
            double vi = input.volatilities[i];
            for (size_t j = 0; j < k; ++j) {
                double tj = input.time_points[j];
                double vj = input.volatilities[j];
                double t_min = std::min(ti, tj);
                double cov = input.forward_strip[i] * input.forward_strip[j] * std::exp(vi * vj * t_min);
                M2 += cov;
            }
        }
        M2 /= static_cast<double>(k * k);

        // Effective Volatility and Forward
        double effective_var = std::log(M2 / (M1 * M1));
        double effective_vol = std::sqrt(std::max(0.0, effective_var) / input.time_to_maturity);
        double F_eff = remaining_weight * M1;
        double K_eff = adjusted_strike;
        double df = std::exp(-input.risk_free_rate * input.time_to_maturity);

        PricingResult result{};

        // Boundary Conditions
        if (K_eff <= 0.0) {
            if (input.is_call) {
                result.price = df * (F_eff - K_eff);
                result.delta = df * remaining_weight;
            } else {
                result.price = 0.0;
                result.delta = 0.0;
            }
        } else {
            double sigma_sqrt_T = effective_vol * std::sqrt(input.time_to_maturity);
            double d1 = (std::log(F_eff / K_eff) + 0.5 * effective_var) / sigma_sqrt_T;
            double d2 = d1 - sigma_sqrt_T;

            if (input.is_call) {
                result.price = df * (F_eff * norm_cdf(d1) - K_eff * norm_cdf(d2));
                result.delta = df * remaining_weight * norm_cdf(d1);
            } else {
                result.price = df * (K_eff * norm_cdf(-d2) - F_eff * norm_cdf(-d1));
                result.delta = -df * remaining_weight * norm_cdf(-d1);
            }

            result.gamma = df * norm_pdf(d1) / (F_eff * sigma_sqrt_T);
            result.vega = df * F_eff * std::sqrt(input.time_to_maturity) * norm_pdf(d1);
            result.theta = -(df * F_eff * norm_pdf(d1) * effective_vol) / (2.0 * std::sqrt(input.time_to_maturity));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        result.latency_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        return result;
    }
};

} // namespace flux::pricing
