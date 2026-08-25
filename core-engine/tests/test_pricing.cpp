#include <iostream>
#include <cassert>
#include <cmath>
#include "../include/asian_engine.hpp"
#include "../include/crack_spread_engine.hpp"
#include "../include/var_engine.hpp"
#include "../include/physical_logistics_engine.hpp"
#include "../include/xva_engine.hpp"

void test_asian_option_boundaries() {
    std::cout << "[TEST] Asian Option Boundary Conditions...";
    flux::pricing::AsianPricerInput input{};
    input.is_call = true;
    input.strike = 50.0;
    input.risk_free_rate = 0.05;
    input.time_to_maturity = 0.5;
    input.total_fixings_count = 10;
    input.realized_count = 5;
    input.remaining_count = 5;
    
    for (size_t i = 0; i < 5; ++i) {
        input.realized_fixings[i] = 100.0;
        input.forward_strip[i] = 100.0;
        input.time_points[i] = 0.1 * (i + 1);
        input.volatilities[i] = 0.20;
    }

    auto res = flux::pricing::TurnbullWakemanAsianKernel::price(input);
    assert(res.price > 40.0 && "Deep ITM Asian option price should reflect intrinsic value");
    assert(res.delta > 0.0 && "Delta must be strictly positive");
    assert(res.gamma >= 0.0 && "Gamma must be non-negative");
    assert(res.vega >= 0.0 && "Vega must be non-negative");
    std::cout << " PASSED\n";
}

void test_crack_spread_pricing() {
    std::cout << "[TEST] Kirk Crack Spread Option Pricing...";
    flux::pricing::CrackSpreadInput input{
        .is_call = true,
        .forward_product = 100.0,
        .forward_crude = 80.0,
        .strike_spread = 20.0,
        .vol_product = 0.30,
        .vol_crude = 0.25,
        .correlation = 0.80,
        .risk_free_rate = 0.05,
        .time_to_maturity = 0.25
    };

    auto res = flux::pricing::KirkCrackSpreadKernel::price(input);
    assert(res.price > 0.0 && "ATM Spread Call price must be positive");
    assert(res.delta_product > 0.0 && "Product Delta must be positive for Call");
    assert(res.delta_crude < 0.0 && "Crude Delta must be negative for Product-Crude Call");
    std::cout << " PASSED\n";
}

void test_var_and_expected_shortfall() {
    std::cout << "[TEST] VaR and Expected Shortfall Monotonicity...";
    std::vector<double> pnls = {
        -1000.0, -800.0, -600.0, -400.0, -200.0, 0.0, 100.0, 200.0, 300.0, 400.0,
        -950.0, -750.0, -550.0, -350.0, -150.0, 50.0, 150.0, 250.0, 350.0, 450.0
    };
    for (int i = 0; i < 80; ++i) pnls.push_back(100.0 * (i + 1));

    auto res = flux::risk::RiskEngine::compute_var_and_es(pnls);
    assert(res.var_99_1d >= res.var_95_1d && "99% VaR must be greater than or equal to 95% VaR");
    assert(res.expected_shortfall_99 >= res.var_99_1d && "Expected Shortfall must exceed VaR at same confidence");
    assert(res.var_99_10d > res.var_99_1d && "10-day VaR must scale with time");
    std::cout << " PASSED\n";
}

void test_crude_blending_conservation() {
    std::cout << "[TEST] Crude Blending Physical Mass Conservation...";
    flux::logistics::CrudeBlendingInput blend{
        .light_sweet_bbl = 500000.0,
        .light_sweet_api = 40.0,
        .light_sweet_sulfur_pct = 0.20,
        .heavy_sour_bbl = 500000.0,
        .heavy_sour_api = 20.0,
        .heavy_sour_sulfur_pct = 3.00
    };

    auto res = flux::logistics::PhysicalLogisticsEngine::compute_crude_blend(blend);
    assert(res.total_volume_bbl == 1000000.0 && "Volume must be strictly additive");
    assert(res.blended_api_gravity > 20.0 && res.blended_api_gravity < 40.0);
    assert(res.blended_sulfur_pct > 0.20 && res.blended_sulfur_pct < 3.00);
    std::cout << " PASSED\n";
}

void test_xva_non_negativity() {
    std::cout << "[TEST] XVA Counterparty Metrics...";
    flux::xva::XVAParameters params{
        .counterparty_hazard_rate = 0.02,
        .own_hazard_rate = 0.01,
        .funding_spread = 0.005,
        .recovery_rate = 0.40,
        .risk_free_rate = 0.04,
        .time_grid = {0.5, 1.0},
        .expected_positive_exposure = {100000.0, 100000.0},
        .expected_negative_exposure = {50000.0, 50000.0}
    };

    auto res = flux::xva::XVAMonteCarloKernel::compute_xva(params);
    assert(res.cva_usd > 0.0 && "CVA must be positive");
    assert(res.dva_usd > 0.0 && "DVA must be positive");
    assert(res.fva_usd > 0.0 && "FVA must be positive");
    std::cout << " PASSED\n";
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  RUNNING FLUX C++ UNIT & BOUNDARY TEST SUITE            \n";
    std::cout << "=========================================================\n";

    test_asian_option_boundaries();
    test_crack_spread_pricing();
    test_var_and_expected_shortfall();
    test_crude_blending_conservation();
    test_xva_non_negativity();

    std::cout << "=========================================================\n";
    std::cout << "  ALL C++ UNIT TESTS PASSED (5 / 5)                      \n";
    std::cout << "=========================================================\n";
    return 0;
}
