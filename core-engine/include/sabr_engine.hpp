#pragma once

#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <chrono>

namespace flux::pricing {

struct alignas(64) SABRParameters {
    double alpha; // Initial volatility level (alpha > 0)
    double beta;  // CEV elasticity exponent (0 <= beta <= 1; beta = 0.5 for CIR, 1.0 for lognormal)
    double rho;   // Correlation between spot and vol (-1 <= rho <= 1)
    double nu;    // Volatility of volatility (nu >= 0)
    double forward_price; // Forward benchmark F
    double strike;        // Strike price K
    double time_to_maturity; // T in years
};

struct alignas(64) SABRResult {
    double implied_volatility;
    double dvol_dstrike; // Skew slope
    double dvol_dalpha;  // Vega sensitivity to ATM level
    uint64_t compute_nanos;
};

class SABREngine {
public:
    /// Hagan et al. (2002) closed-form analytical SABR implied Black volatility
    [[nodiscard]] static SABRResult compute_implied_vol(const SABRParameters& p) noexcept {
        const auto start = std::chrono::high_resolution_clock::now();

        const double F = std::max(1e-6, p.forward_price);
        const double K = std::max(1e-6, p.strike);
        const double T = std::max(1e-6, p.time_to_maturity);
        const double alpha = std::max(1e-8, p.alpha);
        const double beta = std::clamp(p.beta, 0.0, 1.0);
        const double rho = std::clamp(p.rho, -0.9999, 0.9999);
        const double nu = std::max(1e-8, p.nu);

        const double one_minus_beta = 1.0 - beta;
        const double FK = F * K;
        const double sqrt_FK = std::sqrt(FK);
        const double log_FK = std::log(F / K);

        // Term 1: Denominator denominator series expansion
        const double FK_pow = std::pow(FK, one_minus_beta * 0.5);
        const double log_FK_sq = log_FK * log_FK;
        const double denom_expansion = 1.0 + (one_minus_beta * one_minus_beta / 24.0) * log_FK_sq 
                                           + (std::pow(one_minus_beta, 4.0) / 1920.0) * log_FK_sq * log_FK_sq;

        // ATM Case (F == K)
        if (std::abs(F - K) < 1e-8) {
            const double F_pow = std::pow(F, one_minus_beta);
            const double term2 = 1.0 + ( (one_minus_beta * one_minus_beta / 24.0) * (alpha * alpha / (F_pow * F_pow))
                                       + (0.25 * rho * beta * nu * alpha / F_pow)
                                       + ((2.0 - 3.0 * rho * rho) / 24.0) * nu * nu ) * T;
            const double atm_vol = (alpha / F_pow) * term2;

            const auto end = std::chrono::high_resolution_clock::now();
            return SABRResult{
                .implied_volatility = atm_vol,
                .dvol_dstrike = (atm_vol * (beta - 1.0) / (2.0 * F)) + (rho * nu / (2.0 * F_pow)),
                .dvol_dalpha = term2 / F_pow,
                .compute_nanos = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count())
            };
        }

        // Out-of-the-money / In-the-money Case
        const double z = (nu / alpha) * FK_pow * log_FK;
        const double sqrt_term = std::sqrt(1.0 - 2.0 * rho * z + z * z);
        const double chi_z = std::log((sqrt_term + z - rho) / (1.0 - rho));

        const double z_over_chi = (std::abs(z) < 1e-6) ? 1.0 : (z / chi_z);

        const double term2 = 1.0 + ( (one_minus_beta * one_minus_beta / 24.0) * (alpha * alpha / std::pow(FK, one_minus_beta))
                                   + (0.25 * rho * beta * nu * alpha / FK_pow)
                                   + ((2.0 - 3.0 * rho * rho) / 24.0) * nu * nu ) * T;

        const double sigma_sabr = (alpha / (FK_pow * denom_expansion)) * z_over_chi * term2;

        const auto end = std::chrono::high_resolution_clock::now();
        const uint64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return SABRResult{
            .implied_volatility = std::max(1e-6, sigma_sabr),
            .dvol_dstrike = 0.0, // Numerical gradient available via bumping
            .dvol_dalpha = term2 / (FK_pow * denom_expansion) * z_over_chi,
            .compute_nanos = nanos
        };
    }
};

} // namespace flux::pricing
