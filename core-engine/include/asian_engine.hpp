#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <stdexcept>
#include <chrono>

namespace flux::pricing {

// Cache-line aligned input structure (64 bytes alignment to eliminate L1 cache line splitting)
struct alignas(64) AsianPricerInput {
    bool is_call;
    double strike;
    double risk_free_rate;
    double time_to_maturity;
    size_t total_fixings_count;
    size_t realized_count;
    size_t remaining_count;
    
    // In-place fixed stack arrays avoiding heap allocation (max 32 fixings in a monthly strip)
    std::array<double, 32> forward_strip;
    std::array<double, 32> time_points;
    std::array<double, 32> volatilities;
    std::array<double, 32> realized_fixings;
};

struct alignas(64) PricingResult {
    double price;
    double delta;
    double gamma;
    double vega;
    double theta;
    uint64_t latency_nanos;
};

class TurnbullWakemanAsianKernel {
public:
    // Cody's rational Chebyshev minimax polynomial approximation for normal CDF
    static inline double fast_norm_cdf(double z) noexcept {
        if (z > 6.0) return 1.0;
        if (z < -6.0) return 0.0;

        static constexpr double p0 = 220.2068679123761;
        static constexpr double p1 = 221.4315995866529;
        static constexpr double p2 = 112.0792914978709;
        static constexpr double p3 = 33.91286607838300;
        static constexpr double p4 = 6.373962203431650;
        static constexpr double p5 = 0.7003830644436881;
        static constexpr double p6 = 0.03526249659989109;

        static constexpr double q0 = 440.4137358247522;
        static constexpr double q1 = 793.8265125199484;
        static constexpr double q2 = 637.3336333788311;
        static constexpr double q3 = 296.5642487796737;
        static constexpr double q4 = 86.77889516549816;
        static constexpr double q5 = 16.06417757920695;
        static constexpr double q6 = 1.755667163182642;
        static constexpr double q7 = 0.08838834764831844;

        double x = std::abs(z);
        double num = ((((((p6 * x + p5) * x + p4) * x + p3) * x + p2) * x + p1) * x + p0);
        double den = (((((((q7 * x + q6) * x + q5) * x + q4) * x + q3) * x + q2) * x + q1) * x + q0);
        double erfc_approx = std::exp(-0.5 * x * x) * (num / den);

        return z >= 0.0 ? (1.0 - erfc_approx) : erfc_approx;
    }

    static inline double fast_norm_pdf(double x) noexcept {
        static constexpr double INV_SQRT_2PI = 0.3989422804014327;
        return INV_SQRT_2PI * std::exp(-0.5 * x * x);
    }

    [[nodiscard]] static PricingResult price(const AsianPricerInput& input) noexcept {
        const auto start_time = std::chrono::high_resolution_clock::now();

        const size_t m = input.realized_count;
        const size_t N = input.total_fixings_count;
        const size_t k = input.remaining_count;

        if (m + k != N || k == 0 || N == 0) [[unlikely]] {
            return PricingResult{};
        }

        // 1. Calculate Realized Component via unrolled loop
        double realized_sum = 0.0;
        for (size_t i = 0; i < m; ++i) {
            realized_sum += input.realized_fixings[i];
        }

        const double remaining_weight = static_cast<double>(k) / static_cast<double>(N);
        const double adjusted_strike = input.strike - (m > 0 ? (realized_sum / static_cast<double>(N)) : 0.0);

        // 2. Compute 1st and 2nd Moments of Future Average
        double M1 = 0.0;
        for (size_t i = 0; i < k; ++i) {
            M1 += input.forward_strip[i];
        }
        M1 /= static_cast<double>(k);

        double M2 = 0.0;
        for (size_t i = 0; i < k; ++i) {
            const double ti = input.time_points[i];
            const double vi = input.volatilities[i];
            const double fi = input.forward_strip[i];
            for (size_t j = 0; j < k; ++j) {
                const double t_min = std::min(ti, input.time_points[j]);
                const double cov = fi * input.forward_strip[j] * std::exp(vi * input.volatilities[j] * t_min);
                M2 += cov;
            }
        }
        M2 /= static_cast<double>(k * k);

        // Effective Volatility and Effective Forward
        const double ttm = std::max(1e-6, input.time_to_maturity);
        const double effective_var = std::log(std::max(1e-8, M2 / (M1 * M1)));
        const double effective_vol = std::sqrt(std::max(0.0, effective_var) / ttm);
        const double F_eff = std::max(1e-8, remaining_weight * M1);
        const double K_eff = adjusted_strike;
        const double df = std::exp(-input.risk_free_rate * ttm);

        PricingResult result{};

        if (K_eff <= 0.0) [[unlikely]] {
            if (input.is_call) {
                result.price = df * (F_eff - K_eff);
                result.delta = df * remaining_weight;
            } else {
                result.price = 0.0;
                result.delta = 0.0;
            }
        } else {
            const double sigma_sqrt_T = std::max(1e-8, effective_vol * std::sqrt(ttm));
            const double d1 = (std::log(F_eff / K_eff) + 0.5 * effective_var) / sigma_sqrt_T;
            const double d2 = d1 - sigma_sqrt_T;
            const double nd1 = fast_norm_cdf(d1);
            const double nd2 = fast_norm_cdf(d2);
            const double npdf_d1 = fast_norm_pdf(d1);

            if (input.is_call) {
                result.price = df * (F_eff * nd1 - K_eff * nd2);
                result.delta = df * remaining_weight * nd1;
            } else {
                result.price = df * (K_eff * (1.0 - nd2) - F_eff * (1.0 - nd1));
                result.delta = -df * remaining_weight * (1.0 - nd1);
            }

            result.gamma = (df * npdf_d1) / (F_eff * sigma_sqrt_T);
            result.vega = df * F_eff * std::sqrt(ttm) * npdf_d1;
            result.theta = -(df * F_eff * npdf_d1 * effective_vol) / (2.0 * std::sqrt(ttm));
        }

        const auto end_time = std::chrono::high_resolution_clock::now();
        result.latency_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        return result;
    }
};

} // namespace flux::pricing
