#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include "../include/asian_engine.hpp"
#include "../include/crack_spread_engine.hpp"
#include "../include/var_engine.hpp"
#include "../include/physical_logistics_engine.hpp"
#include "../include/xva_engine.hpp"

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  FLUX WORLD-CLASS FAANG/QUANT PRICING & RISK BENCHMARK  \n";
    std::cout << "=========================================================\n\n";

    // 1. Asian Option (APO) Benchmark
    std::cout << "1. Pricing Dated Brent Asian Average Price Option (APO)...\n";
    
    flux::pricing::AsianPricerInput asian_input{};
    asian_input.is_call = true;
    asian_input.strike = 82.50;
    asian_input.risk_free_rate = 0.045;
    asian_input.time_to_maturity = 0.25;
    asian_input.total_fixings_count = 21;
    asian_input.realized_count = 5;
    asian_input.remaining_count = 16;
    
    // Fill realized stack array
    asian_input.realized_fixings = {80.20, 81.10, 81.75, 82.00, 82.40};
    
    // Fill remaining forward strip stack array
    for (size_t i = 0; i < 16; ++i) {
        asian_input.forward_strip[i] = 82.80 + i * 0.05;
        asian_input.time_points[i] = 0.20 + (i * 0.05 / 16.0);
        asian_input.volatilities[i] = 0.28;
    }

    // Warm-up cache
    for (int i = 0; i < 1000; ++i) {
        auto _ = flux::pricing::TurnbullWakemanAsianKernel::price(asian_input);
    }

    auto asian_res = flux::pricing::TurnbullWakemanAsianKernel::price(asian_input);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "   - Call Premium:      $" << asian_res.price << " / bbl\n";
    std::cout << "   - Delta:              " << asian_res.delta << "\n";
    std::cout << "   - Gamma:              " << asian_res.gamma << "\n";
    std::cout << "   - Vega:               " << asian_res.vega << "\n";
    std::cout << "   - Execution Latency:  " << asian_res.latency_nanos << " ns ("
              << (asian_res.latency_nanos / 1000.0) << " µs)\n\n";

    // 2. Crack Spread Option Benchmark
    std::cout << "2. Pricing Gasoil Crack Spread Option (Gasoil vs. Brent)...\n";
    
    flux::pricing::CrackSpreadInput crack_input{
        .is_call = true,
        .forward_product = 98.50,
        .forward_crude = 82.50,
        .strike_spread = 15.00,
        .vol_product = 0.32,
        .vol_crude = 0.28,
        .correlation = 0.85,
        .risk_free_rate = 0.045,
        .time_to_maturity = 0.50
    };

    auto crack_res = flux::pricing::KirkCrackSpreadKernel::price(crack_input);

    std::cout << "   - Crack Call Premium: $" << crack_res.price << " / bbl\n";
    std::cout << "   - Product Delta:      " << crack_res.delta_product << "\n";
    std::cout << "   - Crude Delta:        " << crack_res.delta_crude << "\n";
    std::cout << "   - Execution Latency:  " << crack_res.latency_nanos << " ns ("
              << (crack_res.latency_nanos / 1000.0) << " µs)\n\n";

    // 3. Historical VaR & Expected Shortfall Benchmark
    std::cout << "3. Full Portfolio Historical VaR & Expected Shortfall (500 Scenarios)...\n";
    
    std::vector<double> simulated_pnls;
    simulated_pnls.reserve(500);
    std::mt19937 rng(42);
    std::normal_distribution<double> dist(1250.0, 45000.0);
    for (int i = 0; i < 500; ++i) simulated_pnls.push_back(dist(rng));
    simulated_pnls[12] = -185000.0;
    simulated_pnls[88] = -215000.0;

    auto risk_res = flux::risk::RiskEngine::compute_var_and_es(simulated_pnls);

    std::cout << "   - 99% 1-Day VaR:            $" << risk_res.var_99_1d << "\n";
    std::cout << "   - Expected Shortfall 97.5%:  $" << risk_res.expected_shortfall_975 << "\n";
    std::cout << "   - Worst Case Stress Loss:    $" << risk_res.worst_case_loss << "\n\n";

    // 4. Physical Logistics & Crude Blending Optimization
    std::cout << "4. Physical Logistics: VLCC Demurrage & Crude Blending Optimization...\n";
    
    flux::logistics::VesselFixture fixture{
        .vessel_name = "DHT HAWK (VLCC)",
        .charter_party_type = "SHELLVOY5",
        .parcel_size_bbl = 2000000.0,
        .laytime_allowed_hours = 72.0,
        .demurrage_rate_per_day_usd = 65000.0,
        .actual_laytime_used_hours = 96.0
    };
    auto dem_res = flux::logistics::PhysicalLogisticsEngine::compute_demurrage(fixture);
    std::cout << "   - Demurrage Incurred:        $" << dem_res.demurrage_incurred_usd << "\n";

    flux::logistics::CrudeBlendingInput blend_input{
        .light_sweet_bbl = 600000.0,
        .light_sweet_api = 41.5,
        .light_sweet_sulfur_pct = 0.15,
        .heavy_sour_bbl = 400000.0,
        .heavy_sour_api = 22.0,
        .heavy_sour_sulfur_pct = 3.20
    };
    auto blend_res = flux::logistics::PhysicalLogisticsEngine::compute_crude_blend(blend_input);
    std::cout << "   - Blended Volume:            " << blend_res.total_volume_bbl << " bbl\n";
    std::cout << "   - Blended API Gravity:       " << blend_res.blended_api_gravity << " API\n";
    std::cout << "   - Blending Compute Latency:  " << blend_res.compute_nanos << " ns\n\n";

    // 5. Advanced XVA (CVA / DVA / FVA)
    std::cout << "5. Advanced XVA: Multi-Curve Counterparty Default & Funding Adjustments...\n";
    
    flux::xva::XVAParameters xva_params{
        .counterparty_hazard_rate = 0.025,
        .own_hazard_rate = 0.010,
        .funding_spread = 0.0065,
        .recovery_rate = 0.40,
        .risk_free_rate = 0.045,
        .time_grid = {0.25, 0.50, 0.75, 1.0, 2.0},
        .expected_positive_exposure = {450000.0, 680000.0, 820000.0, 710000.0, 390000.0},
        .expected_negative_exposure = {120000.0, 190000.0, 250000.0, 210000.0, 95000.0}
    };
    auto xva_res = flux::xva::XVAMonteCarloKernel::compute_xva(xva_params);

    std::cout << "   - Net Total XVA Adjustment:  $" << xva_res.total_xva_adjustment_usd << "\n";
    std::cout << "   - XVA Compute Latency:       " << xva_res.compute_nanos << " ns ("
              << (xva_res.compute_nanos / 1000.0) << " µs)\n";

    std::cout << "\n=========================================================\n";
    std::cout << "  ALL FAANG-TIER BENCHMARKS EXECUTED SUCCESSFULLY        \n";
    std::cout << "=========================================================\n";

    return 0;
}
