#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <chrono>

namespace flux::risk {

template <size_t N_ASSETS>
struct MultiAssetPortfolioInput {
    std::array<double, N_ASSETS> spot_prices;
    std::array<double, N_ASSETS> drift_rates;
    std::array<double, N_ASSETS> volatilities;
    std::array<double, N_ASSETS> portfolio_positions; // Units held per asset
    std::array<std::array<double, N_ASSETS>, N_ASSETS> correlation_matrix;
    double time_horizon_years;
    size_t num_simulated_paths;
};

struct MonteCarloVaRResult {
    double portfolio_initial_value;
    double var_95_usd;
    double var_99_usd;
    double expected_shortfall_975_usd;
    double worst_case_loss_usd;
    double mean_simulated_pnl;
    size_t paths_evaluated;
    uint64_t compute_nanos;
};

class CholeskyCorrelatedMonteCarloEngine {
public:
    /// Computes Cholesky factor L such that L * L^T = CorrelationMatrix
    template <size_t N>
    static std::array<std::array<double, N>, N> compute_cholesky(
        const std::array<std::array<double, N>, N>& corr
    ) {
        std::array<std::array<double, N>, N> L{};

        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j <= i; ++j) {
                double sum = 0.0;
                for (size_t k = 0; k < j; ++k) {
                    sum += L[i][k] * L[j][k];
                }

                if (i == j) {
                    double val = corr[i][i] - sum;
                    if (val <= 0.0) {
                        L[i][j] = 1e-6; // Ensure positive semi-definite
                    } else {
                        L[i][j] = std::sqrt(val);
                    }
                } else {
                    L[i][j] = (corr[i][j] - sum) / std::max(1e-8, L[j][j]);
                }
            }
        }
        return L;
    }

    /// Simulates correlated terminal asset prices and calculates full portfolio VaR / ES
    template <size_t N>
    static MonteCarloVaRResult evaluate_portfolio_var(
        const MultiAssetPortfolioInput<N>& in,
        unsigned int seed = 42
    ) {
        const auto start = std::chrono::high_resolution_clock::now();

        // 1. Calculate Initial Portfolio Value
        double V0 = 0.0;
        for (size_t i = 0; i < N; ++i) {
            V0 += in.portfolio_positions[i] * in.spot_prices[i];
        }

        // 2. Cholesky Factorization of Correlation Matrix
        const auto L = compute_cholesky<N>(in.correlation_matrix);

        // 3. Monte Carlo Simulation Loop
        std::mt19937_64 rng(seed);
        std::normal_distribution<double> norm(0.0, 1.0);

        const double T = in.time_horizon_years;
        const double sqrt_T = std::sqrt(T);

        std::vector<double> simulated_pnls(in.num_simulated_paths);
        std::array<double, N> Z_uncorr{};
        std::array<double, N> Z_corr{};

        double pnl_sum = 0.0;

        for (size_t p = 0; p < in.num_simulated_paths; ++p) {
            // Draw uncorrelated standard normals
            for (size_t i = 0; i < N; ++i) {
                Z_uncorr[i] = norm(rng);
            }

            // Correlate: Z_corr = L * Z_uncorr
            for (size_t i = 0; i < N; ++i) {
                double corr_z = 0.0;
                for (size_t j = 0; j <= i; ++j) {
                    corr_z += L[i][j] * Z_uncorr[j];
                }
                Z_corr[i] = corr_z;
            }

            // Revalue Portfolio at Horizon T
            double VT = 0.0;
            for (size_t i = 0; i < N; ++i) {
                const double S0 = in.spot_prices[i];
                const double mu = in.drift_rates[i];
                const double sigma = in.volatilities[i];

                const double ST = S0 * std::exp((mu - 0.5 * sigma * sigma) * T + sigma * sqrt_T * Z_corr[i]);
                VT += in.portfolio_positions[i] * ST;
            }

            const double pnl = VT - V0;
            simulated_pnls[p] = pnl;
            pnl_sum += pnl;
        }

        // 4. Quantile Extraction for VaR & Expected Shortfall
        std::sort(simulated_pnls.begin(), simulated_pnls.end());
        const size_t num_p = in.num_simulated_paths;

        const size_t idx_95 = static_cast<size_t>(num_p * 0.05);
        const size_t idx_975 = static_cast<size_t>(num_p * 0.025);
        const size_t idx_99 = static_cast<size_t>(num_p * 0.01);

        const double var_95 = -simulated_pnls[idx_95];
        const double var_99 = -simulated_pnls[idx_99];

        double sum_es_975 = 0.0;
        for (size_t i = 0; i <= idx_975; ++i) {
            sum_es_975 += simulated_pnls[i];
        }
        const double es_975 = -(sum_es_975 / static_cast<double>(idx_975 + 1));

        const auto end = std::chrono::high_resolution_clock::now();
        const uint64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return MonteCarloVaRResult{
            .portfolio_initial_value = V0,
            .var_95_usd = std::max(0.0, var_95),
            .var_99_usd = std::max(0.0, var_99),
            .expected_shortfall_975_usd = std::max(0.0, es_975),
            .worst_case_loss_usd = -simulated_pnls.front(),
            .mean_simulated_pnl = pnl_sum / static_cast<double>(num_p),
            .paths_evaluated = num_p,
            .compute_nanos = nanos
        };
    }
};

} // namespace flux::risk
