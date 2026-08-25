#pragma once

#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace flux::risk {

struct RiskMetricsResult {
    double var_95_1d;
    double var_99_1d;
    double var_99_10d;
    double expected_shortfall_975;
    double expected_shortfall_99;
    double worst_case_loss;
    size_t scenario_count;
};

class RiskEngine {
public:
    static RiskMetricsResult compute_var_and_es(std::vector<double> scenario_pnls) {
        if (scenario_pnls.empty()) {
            throw std::invalid_argument("Scenario PnL vector cannot be empty.");
        }

        const size_t N = scenario_pnls.size();
        std::sort(scenario_pnls.begin(), scenario_pnls.end());

        size_t idx_95 = static_cast<size_t>(std::floor(N * 0.05));
        size_t idx_975 = static_cast<size_t>(std::floor(N * 0.025));
        size_t idx_99 = static_cast<size_t>(std::floor(N * 0.010));

        double var_95 = -scenario_pnls[idx_95];
        double var_99 = -scenario_pnls[idx_99];
        
        static constexpr double SQRT_10 = 3.1622776601683795;
        double var_99_10d = var_99 * SQRT_10;

        double sum_es_975 = 0.0;
        for (size_t i = 0; i <= idx_975; ++i) {
            sum_es_975 += scenario_pnls[i];
        }
        double es_975 = -(sum_es_975 / static_cast<double>(idx_975 + 1));

        double sum_es_99 = 0.0;
        for (size_t i = 0; i <= idx_99; ++i) {
            sum_es_99 += scenario_pnls[i];
        }
        double es_99 = -(sum_es_99 / static_cast<double>(idx_99 + 1));

        return RiskMetricsResult{
            .var_95_1d = std::max(0.0, var_95),
            .var_99_1d = std::max(0.0, var_99),
            .var_99_10d = std::max(0.0, var_99_10d),
            .expected_shortfall_975 = std::max(0.0, es_975),
            .expected_shortfall_99 = std::max(0.0, es_99),
            .worst_case_loss = -scenario_pnls.front(),
            .scenario_count = N
        };
    }
};

} // namespace flux::risk
