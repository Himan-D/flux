#pragma once

#include <cmath>
#include <algorithm>
#include <chrono>
#include "asian_engine.hpp"

namespace flux::pricing {

// 1. Standard 1:1 Crack Spread Input (Asset 1 - Asset 2 - Strike)
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
    double kirk_volatility;
    uint64_t latency_nanos;
};

class KirkCrackSpreadKernel {
public:
    [[nodiscard]] static CrackSpreadResult price(const CrackSpreadInput& in) noexcept {
        const auto start_time = std::chrono::high_resolution_clock::now();

        const double F1 = in.forward_product;
        const double F2 = in.forward_crude;
        const double K = in.strike_spread;
        const double T = std::max(1e-6, in.time_to_maturity);
        const double r = in.risk_free_rate;
        const double df = std::exp(-r * T);

        const double F2_prime = F2 + K;
        if (F2_prime <= 0.0 || F1 <= 0.0) [[unlikely]] {
            return CrackSpreadResult{};
        }

        const double w = F2 / F2_prime;
        const double v1 = in.vol_product;
        const double v2 = in.vol_crude;
        const double rho = in.correlation;

        const double v_kirk_sq = v1 * v1 - 2.0 * rho * v1 * v2 * w + (v2 * w) * (v2 * w);
        const double v_kirk = std::sqrt(std::max(1e-8, v_kirk_sq));
        const double sigma_sqrt_T = v_kirk * std::sqrt(T);

        const double d1 = (std::log(F1 / F2_prime) + 0.5 * v_kirk_sq * T) / sigma_sqrt_T;
        const double d2 = d1 - sigma_sqrt_T;
        const double nd1 = TurnbullWakemanAsianKernel::fast_norm_cdf(d1);
        const double nd2 = TurnbullWakemanAsianKernel::fast_norm_cdf(d2);
        const double npdf_d1 = TurnbullWakemanAsianKernel::fast_norm_pdf(d1);

        CrackSpreadResult res{};
        res.kirk_volatility = v_kirk;

        if (in.is_call) {
            res.price = df * (F1 * nd1 - F2_prime * nd2);
            res.delta_product = df * nd1;
            res.delta_crude = -df * nd2 * (1.0 + (F2 * v2 * (v2 * w - rho * v1)) / (F2_prime * v_kirk_sq));
        } else {
            res.price = df * (F2_prime * (1.0 - nd2) - F1 * (1.0 - nd1));
            res.delta_product = -df * (1.0 - nd1);
            res.delta_crude = df * (1.0 - nd2) * (1.0 + (F2 * v2 * (v2 * w - rho * v1)) / (F2_prime * v_kirk_sq));
        }

        res.cross_vega = df * F1 * std::sqrt(T) * npdf_d1;

        const auto end_time = std::chrono::high_resolution_clock::now();
        res.latency_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        return res;
    }
};

// 2. Standard 3:2:1 Refinery Crack Spread Input (2 bbl Gasoline + 1 bbl Heating Oil - 3 bbl Crude)
struct alignas(64) RefineryCrackSpreadInput {
    bool is_call;
    double weight_gasoline;   // e.g. 2.0 / 3.0
    double weight_distillate; // e.g. 1.0 / 3.0
    double weight_crude;      // e.g. 1.0

    double forward_gasoline;   // RBOB in $/bbl
    double forward_distillate; // ULSD / Heating Oil in $/bbl
    double forward_crude;      // WTI / Brent in $/bbl

    double vol_gasoline;
    double vol_distillate;
    double vol_crude;

    double corr_gas_dist;  // rho(Gasoline, Distillate)
    double corr_gas_crude; // rho(Gasoline, Crude)
    double corr_dist_crude;// rho(Distillate, Crude)

    double strike_spread;
    double risk_free_rate;
    double time_to_maturity;
};

struct alignas(64) RefineryCrackSpreadResult {
    double price;
    double delta_gasoline;
    double delta_distillate;
    double delta_crude;
    double composite_volatility;
    double cross_vega;
    uint64_t latency_nanos;
};

class GeneralizedKirkCrackSpreadEngine {
public:
    [[nodiscard]] static RefineryCrackSpreadResult price(const RefineryCrackSpreadInput& in) noexcept {
        const auto start_time = std::chrono::high_resolution_clock::now();

        const double w_g = in.weight_gasoline;
        const double w_d = in.weight_distillate;
        const double w_c = in.weight_crude;

        const double F_g = in.forward_gasoline;
        const double F_d = in.forward_distillate;
        const double F_c = in.forward_crude;

        const double v_g = in.vol_gasoline;
        const double v_d = in.vol_distillate;
        const double v_c = in.vol_crude;

        const double rho_gd = in.corr_gas_dist;
        const double rho_gc = in.corr_gas_crude;
        const double rho_dc = in.corr_dist_crude;

        const double K = in.strike_spread;
        const double T = std::max(1e-6, in.time_to_maturity);
        const double r = in.risk_free_rate;
        const double df = std::exp(-r * T);

        // 1. Synthesize Refined Product Basket (Asset 1 = w_g * F_g + w_d * F_d)
        const double F_prod = w_g * F_g + w_d * F_d;
        if (F_prod <= 0.0 || F_c <= 0.0) [[unlikely]] {
            return RefineryCrackSpreadResult{};
        }

        const double term_g = w_g * F_g * v_g;
        const double term_d = w_d * F_d * v_d;
        const double var_prod = (term_g * term_g + term_d * term_d + 2.0 * term_g * term_d * rho_gd) / (F_prod * F_prod);
        const double v_prod = std::sqrt(std::max(1e-8, var_prod));

        const double cov_prod_crude = (w_g * F_g * v_g * rho_gc + w_d * F_d * v_d * rho_dc) * v_c;
        const double rho_eff = cov_prod_crude / std::max(1e-8, (F_prod * v_prod * v_c));

        // 2. Kirk Spread Formulation on Product Basket vs Crude
        const double F_crude_weighted = w_c * F_c;
        const double F2_prime = F_crude_weighted + K;
        if (F2_prime <= 0.0) [[unlikely]] {
            return RefineryCrackSpreadResult{};
        }

        const double w_kirk = F_crude_weighted / F2_prime;
        const double v_kirk_sq = var_prod - 2.0 * rho_eff * v_prod * v_c * w_kirk + (v_c * w_kirk) * (v_c * w_kirk);
        const double v_kirk = std::sqrt(std::max(1e-8, v_kirk_sq));
        const double sigma_sqrt_T = v_kirk * std::sqrt(T);

        const double d1 = (std::log(F_prod / F2_prime) + 0.5 * v_kirk_sq * T) / sigma_sqrt_T;
        const double d2 = d1 - sigma_sqrt_T;
        const double nd1 = TurnbullWakemanAsianKernel::fast_norm_cdf(d1);
        const double nd2 = TurnbullWakemanAsianKernel::fast_norm_cdf(d2);
        const double npdf_d1 = TurnbullWakemanAsianKernel::fast_norm_pdf(d1);

        RefineryCrackSpreadResult res{};
        res.composite_volatility = v_kirk;

        if (in.is_call) {
            res.price = df * (F_prod * nd1 - F2_prime * nd2);
            res.delta_gasoline = df * w_g * nd1;
            res.delta_distillate = df * w_d * nd1;
            res.delta_crude = -df * w_c * nd2;
        } else {
            res.price = df * (F2_prime * (1.0 - nd2) - F_prod * (1.0 - nd1));
            res.delta_gasoline = -df * w_g * (1.0 - nd1);
            res.delta_distillate = -df * w_d * (1.0 - nd1);
            res.delta_crude = df * w_c * (1.0 - nd2);
        }

        res.cross_vega = df * F_prod * std::sqrt(T) * npdf_d1;

        const auto end_time = std::chrono::high_resolution_clock::now();
        res.latency_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        return res;
    }
};

} // namespace flux::pricing
