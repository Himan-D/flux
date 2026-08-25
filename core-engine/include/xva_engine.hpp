#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace flux::xva {

struct XVAParameters {
    double counterparty_hazard_rate; // Constant default intensity lambda (e.g. 0.02 = 200 bps CDS)
    double own_hazard_rate;          // Firm's own credit spread (e.g. 0.01 = 100 bps CDS)
    double funding_spread;           // Funding spread over OIS/SOFR (e.g. 0.0075 = 75 bps)
    double recovery_rate;            // Standard commodity recovery (e.g. 0.40 = 40%)
    double risk_free_rate;           // 0.045
    std::vector<double> time_grid;   // Grid in years: [0.1, 0.25, 0.5, 1.0, 2.0]
    std::vector<double> expected_positive_exposure; // EPE(t_i) in USD
    std::vector<double> expected_negative_exposure; // ENE(t_i) in USD
};

struct XVAMetricsResult {
    double cva_usd;  // Credit Value Adjustment (Risk of counterparty default)
    double dva_usd;  // Debit Value Adjustment (Firm's own default benefit)
    double fva_usd;  // Funding Value Adjustment (Cost of uncollateralized funding)
    double total_xva_adjustment_usd;
    uint64_t compute_nanos;
};

class XVAMonteCarloKernel {
public:
    static XVAMetricsResult compute_xva(const XVAParameters& params) {
        auto start = std::chrono::high_resolution_clock::now();

        const size_t N = params.time_grid.size();
        if (N == 0 || params.expected_positive_exposure.size() < N || params.expected_negative_exposure.size() < N) {
            return XVAMetricsResult{};
        }

        double cva = 0.0;
        double dva = 0.0;
        double fva = 0.0;
        double lgd = std::max(0.0, 1.0 - params.recovery_rate); // Loss Given Default

        double prev_t = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double t = params.time_grid[i];
            double dt = std::max(0.0, t - prev_t);
            double df = std::exp(-params.risk_free_rate * t);

            // Marginal Default Probability of Counterparty: dPD = exp(-lambda*t_prev) - exp(-lambda*t)
            double pd_cpty = std::exp(-params.counterparty_hazard_rate * prev_t) - std::exp(-params.counterparty_hazard_rate * t);
            // Marginal Default Probability of Own Firm
            double pd_own = std::exp(-params.own_hazard_rate * prev_t) - std::exp(-params.own_hazard_rate * t);

            // CVA = LGD * Sum( DF(t) * EPE(t) * dPD_cpty )
            cva += lgd * df * params.expected_positive_exposure[i] * std::max(0.0, pd_cpty);

            // DVA = LGD_own * Sum( DF(t) * ENE(t) * dPD_own )
            dva += lgd * df * params.expected_negative_exposure[i] * std::max(0.0, pd_own);

            // FVA = Sum( DF(t) * (EPE(t) - ENE(t)) * FundingSpread * dt )
            double net_exposure = params.expected_positive_exposure[i] - params.expected_negative_exposure[i];
            fva += df * net_exposure * params.funding_spread * dt;

            prev_t = t;
        }

        double total_xva = -cva + dva - fva;

        auto end = std::chrono::high_resolution_clock::now();
        uint64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return XVAMetricsResult{
            .cva_usd = cva,
            .dva_usd = dva,
            .fva_usd = fva,
            .total_xva_adjustment_usd = total_xva,
            .compute_nanos = nanos
        };
    }
};

} // namespace flux::xva
