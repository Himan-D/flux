#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <cassert>
#include "../include/asian_engine.hpp"
#include "../include/crack_spread_engine.hpp"

using namespace flux::pricing;

// Independent Monte Carlo reference engine for Asian arithmetic options
double monte_carlo_asian_reference(
    double S0, double strike, double r, double sigma, double T,
    size_t fixings, size_t num_paths, unsigned int seed = 42
) {
    std::mt19937_64 rng(seed);
    std::normal_distribution<double> norm(0.0, 1.0);

    const double dt = T / static_cast<double>(fixings);
    const double drift = (r - 0.5 * sigma * sigma) * dt;
    const double vol_sqrt_dt = sigma * std::sqrt(dt);

    double payoff_sum = 0.0;

    for (size_t p = 0; p < num_paths; ++p) {
        double current_S = S0;
        double path_sum = 0.0;

        for (size_t f = 0; f < fixings; ++f) {
            double z = norm(rng);
            current_S *= std::exp(drift + vol_sqrt_dt * z);
            path_sum += current_S;
        }

        double arithmetic_avg = path_sum / static_cast<double>(fixings);
        double payoff = std::max(0.0, arithmetic_avg - strike);
        payoff_sum += payoff;
    }

    double discount = std::exp(-r * T);
    return discount * (payoff_sum / static_cast<double>(num_paths));
}

int main() {
    std::cout << "====================================================================" << std::endl;
    std::cout << "  FLUX MATHEMATICAL VALIDATION & TOLERANCE TEST SUITE               " << std::endl;
    std::cout << "====================================================================" << std::endl;

    // Test Case 1: Turnbull-Wakeman vs 200,000-Path Monte Carlo Reference
    std::cout << "\n[1] Validating Turnbull-Wakeman vs Independent Monte Carlo Reference..." << std::endl;
    
    const double S0 = 82.50;
    const double strike = 82.50;
    const double r = 0.045;
    const double sigma = 0.28;
    const double T = 0.25; // 3 months
    const size_t fixings = 21; // 21 business days

    AsianPricerInput input{};
    input.is_call = true;
    input.strike = strike;
    input.risk_free_rate = r;
    input.time_to_maturity = T;
    input.total_fixings_count = fixings;
    input.realized_count = 0;
    input.remaining_count = fixings;

    const double dt = T / static_cast<double>(fixings);
    for (size_t i = 0; i < fixings; ++i) {
        double t_i = (i + 1) * dt;
        input.time_points[i] = t_i;
        input.forward_strip[i] = S0 * std::exp(r * t_i);
        input.volatilities[i] = sigma;
    }

    PricingResult tw_result = TurnbullWakemanAsianKernel::price(input);
    double mc_price = monte_carlo_asian_reference(S0, strike, r, sigma, T, fixings, 200000, 12345);
    double diff = std::abs(tw_result.price - mc_price);
    double rel_error = diff / mc_price;

    std::cout << std::fixed << std::setprecision(5);
    std::cout << "    • Turnbull-Wakeman Analytical Price: $" << tw_result.price << std::endl;
    std::cout << "    • Monte Carlo (200k paths) Price:    $" << mc_price << std::endl;
    std::cout << "    • Absolute Error:                   $" << diff << std::endl;
    std::cout << "    • Relative Error:                    " << (rel_error * 100.0) << "%" << std::endl;

    if (rel_error > 0.02) { // 2% tolerance on Monte Carlo sampling variance
        std::cerr << "FAIL: Turnbull-Wakeman deviated significantly from Monte Carlo!" << std::endl;
        return 1;
    }
    std::cout << "    -> VALIDATION PASSED: Within Monte Carlo standard error bounds." << std::endl;

    // Test Case 2: Analytical Delta vs Finite-Difference Numerical Delta
    std::cout << "\n[2] Validating Analytical Delta vs Finite-Difference (Bumping S0)..." << std::endl;
    const double eps = 0.001;
    
    AsianPricerInput input_up = input;
    AsianPricerInput input_down = input;
    for (size_t i = 0; i < fixings; ++i) {
        input_up.forward_strip[i] += eps;
        input_down.forward_strip[i] -= eps;
    }
    double price_up = TurnbullWakemanAsianKernel::price(input_up).price;
    double price_down = TurnbullWakemanAsianKernel::price(input_down).price;
    double finite_diff_delta = (price_up - price_down) / (2.0 * eps);

    double delta_diff = std::abs(tw_result.delta - finite_diff_delta);
    std::cout << "    • Analytical Delta:                  " << tw_result.delta << std::endl;
    std::cout << "    • Finite-Difference (Numerical) Δ:   " << finite_diff_delta << std::endl;
    std::cout << "    • Difference:                        " << delta_diff << std::endl;

    if (delta_diff > 0.005) {
        std::cerr << "FAIL: Analytical Delta deviated from finite difference!" << std::endl;
        return 1;
    }
    std::cout << "    -> VALIDATION PASSED: Analytical Greek matches finite-difference gradient." << std::endl;

    std::cout << "\n====================================================================" << std::endl;
    std::cout << "  ALL MATHEMATICAL VALIDATION GATES PASSED SUCCESSFULLY             " << std::endl;
    std::cout << "====================================================================" << std::endl;
    return 0;
}
