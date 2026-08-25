#pragma once

#include <cmath>
#include <algorithm>
#include <chrono>
#include "asian_engine.hpp"

namespace flux::pricing {

struct alignas(64) CrackSpreadInput {
    bool is_call;
    double forward_product;
    double forward_crude;
    double strike_spread;
    double vol_product;
    double vol_crude;
    double correlation;
    double risk_free_rate;
    double time_to_maturity;
};

struct alignas(64) CrackSpreadResult {
    double price;
    double delta_product;
    double delta_crude;
    double cross_vega;
    uint64_t latency_nanos;
};

class KirkCrackSpreadKernel {
public:
    [[nodiscard]] static CrackSpreadResult price(const CrackSpreadInput& input) noexcept {
        const auto start_time = std::chrono::high_resolution_clock::now();

        const double F1 = input.forward_product;
        const double F2 = input.forward_crude;
        const double K = input.strike_spread;
        const double T = input.time_to_maturity;
        const double v1 = input.vol_product;
        const double v2 = input.vol_crude;
        const double rho = input.correlation;
        const double r = input.risk_free_rate;

        const double F2_prime = F2 + K;
        if (F2_prime <= 0.0 || F1 <= 0.0) [[unlikely]] {
            return CrackSpreadResult{};
        }

        const double w = F2 / F2_prime;
        const double v_kirk_sq = v1 * v1 - 2.0 * rho * v1 * v2 * w + (v2 * w) * (v2 * w);
        const double v_kirk = std::sqrt(std::max(1e-8, v_kirk_sq));
        const double sigma_sqrt_T = v_kirk * std::sqrt(T);

        const double d1 = (std::log(F1 / F2_prime) + 0.5 * v_kirk_sq * T) / sigma_sqrt_T;
        const double d2 = d1 - sigma_sqrt_T;
        const double df = std::exp(-r * T);
        const double nd1 = TurnbullWakemanAsianKernel::fast_norm_cdf(d1);
        const double nd2 = TurnbullWakemanAsianKernel::fast_norm_cdf(d2);
        const double npdf_d1 = TurnbullWakemanAsianKernel::fast_norm_pdf(d1);

        CrackSpreadResult res{};
        if (input.is_call) {
            res.price = df * (F1 * nd1 - F2_prime * nd2);
            res.delta_product = df * nd1;
            res.delta_crude = -df * nd2 * (1.0 + (F2 * v2 * (v2 * w - rho * v1)) / (F2_prime * v_kirk_sq));
        } else {
            res.price = df * (F2_prime * (1.0 - nd2) - F1 * (1.0 - nd1));
            res.delta_product = -df * (1.0 - nd1);
            res.delta_crude = df * (1.0 - nd2);
        }

        res.cross_vega = df * F1 * std::sqrt(T) * npdf_d1;

        const auto end_time = std::chrono::high_resolution_clock::now();
        res.latency_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        return res;
    }
};

} // namespace flux::pricing
