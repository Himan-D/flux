#pragma once

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <chrono>

namespace flux::pricing {

struct CrackSpreadInput {
    bool is_call;
    double forward_product; // e.g. Gasoil ($/bbl or $/MT converted)
    double forward_crude;   // e.g. Brent crude ($/bbl)
    double strike_spread;   // Spread strike ($/bbl)
    double vol_product;     // Volatility of product
    double vol_crude;       // Volatility of crude
    double correlation;     // Cross-commodity correlation rho [-1.0, 1.0]
    double risk_free_rate;
    double time_to_maturity;
};

struct CrackSpreadResult {
    double price;
    double delta_product;
    double delta_crude;
    double cross_vega;
    uint64_t latency_nanos;
};

class KirkCrackSpreadKernel {
public:
    static inline double norm_cdf(double x) noexcept {
        return 0.5 * std::erfc(-x * M_SQRT1_2);
    }

    static inline double norm_pdf(double x) noexcept {
        static constexpr double INV_SQRT_2PI = 0.3989422804014327;
        return INV_SQRT_2PI * std::exp(-0.5 * x * x);
    }

    /// Prices a European spread option Max(F1 - F2 - K, 0) using Kirk's Approximation
    static CrackSpreadResult price(const CrackSpreadInput& input) {
        auto start_time = std::chrono::high_resolution_clock::now();

        double F1 = input.forward_product;
        double F2 = input.forward_crude;
        double K = input.strike_spread;
        double T = input.time_to_maturity;
        double v1 = input.vol_product;
        double v2 = input.vol_crude;
        double rho = input.correlation;
        double r = input.risk_free_rate;

        double F2_prime = F2 + K;
        if (F2_prime <= 0.0 || F1 <= 0.0) {
            throw std::invalid_argument("F2 + K or F1 must be strictly positive for Kirk formula.");
        }

        double w = F2 / F2_prime;
        // Kirk adjusted volatility
        double v_kirk_sq = v1 * v1 - 2.0 * rho * v1 * v2 * w + (v2 * w) * (v2 * w);
        double v_kirk = std::sqrt(std::max(1e-8, v_kirk_sq));
        double sigma_sqrt_T = v_kirk * std::sqrt(T);

        double d1 = (std::log(F1 / F2_prime) + 0.5 * v_kirk_sq * T) / sigma_sqrt_T;
        double d2 = d1 - sigma_sqrt_T;
        double df = std::exp(-r * T);

        CrackSpreadResult res{};
        if (input.is_call) {
            res.price = df * (F1 * norm_cdf(d1) - F2_prime * norm_cdf(d2));
            res.delta_product = df * norm_cdf(d1);
            res.delta_crude = -df * norm_cdf(d2) * (1.0 + (F2 * v2 * (v2 * w - rho * v1)) / (F2_prime * v_kirk_sq));
        } else {
            res.price = df * (F2_prime * norm_cdf(-d2) - F1 * norm_cdf(-d1));
            res.delta_product = -df * norm_cdf(-d1);
            res.delta_crude = df * norm_cdf(-d2);
        }

        res.cross_vega = df * F1 * std::sqrt(T) * norm_pdf(d1);

        auto end_time = std::chrono::high_resolution_clock::now();
        res.latency_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        return res;
    }
};

} // namespace flux::pricing
