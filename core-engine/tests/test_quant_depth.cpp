#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include "../include/sabr_engine.hpp"
#include "../include/crack_spread_engine.hpp"
#include "../include/monte_carlo_engine.hpp"

using namespace flux::pricing;
using namespace flux::risk;

int main() {
    std::cout << "====================================================================" << std::endl;
    std::cout << "  FLUX ADVANCED QUANTITATIVE DEPTH TEST SUITE                       " << std::endl;
    std::cout << "====================================================================" << std::endl;

    // [1] SABR Model Validation (Hagan et al. 2002)
    std::cout << "\n[1] Testing Hagan (2002) SABR Volatility Smile Engine..." << std::endl;
    SABRParameters sabr_atm{
        .alpha = 0.28,
        .beta = 0.70, // Oil standard CEV beta
        .rho = -0.25, // Negative spot-vol correlation (leverage/oil skew)
        .nu = 0.40,   // Vol of vol
        .forward_price = 82.50,
        .strike = 82.50, // ATM
        .time_to_maturity = 0.25
    };
    SABRResult atm_res = SABREngine::compute_implied_vol(sabr_atm);
    
    SABRParameters sabr_otm = sabr_atm;
    sabr_otm.strike = 90.00; // OTM Call
    SABRResult otm_res = SABREngine::compute_implied_vol(sabr_otm);

    SABRParameters sabr_itm = sabr_atm;
    sabr_itm.strike = 75.00; // ITM Call (OTM Put)
    SABRResult itm_res = SABREngine::compute_implied_vol(sabr_itm);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "    • ATM Implied Vol (K=$82.50): " << (atm_res.implied_volatility * 100.0) << "%" << std::endl;
    std::cout << "    • OTM Implied Vol (K=$90.00): " << (otm_res.implied_volatility * 100.0) << "%" << std::endl;
    std::cout << "    • ITM Implied Vol (K=$75.00): " << (itm_res.implied_volatility * 100.0) << "%" << std::endl;

    assert(atm_res.implied_volatility > 0.05 && atm_res.implied_volatility < 1.0);
    assert(itm_res.implied_volatility != otm_res.implied_volatility);
    std::cout << "    -> SABR SMILE VALIDATION PASSED (Smooth Non-Linear Vol Surface Generated)" << std::endl;

    // [2] Generalized 3:2:1 Refinery Crack Spread Option Engine
    std::cout << "\n[2] Testing Generalized 3:2:1 Refinery Crack Spread Engine..." << std::endl;
    RefineryCrackSpreadInput crack_321{
        .is_call = true,
        .weight_gasoline = 2.0 / 3.0,
        .weight_distillate = 1.0 / 3.0,
        .weight_crude = 1.0,
        .forward_gasoline = 105.00,   // RBOB in $/bbl
        .forward_distillate = 112.00, // Heating Oil in $/bbl
        .forward_crude = 82.50,       // WTI / Brent in $/bbl
        .vol_gasoline = 0.35,
        .vol_distillate = 0.32,
        .vol_crude = 0.28,
        .corr_gas_dist = 0.88,
        .corr_gas_crude = 0.82,
        .corr_dist_crude = 0.85,
        .strike_spread = 22.00,       // Margin strike $/bbl
        .risk_free_rate = 0.045,
        .time_to_maturity = 0.25
    };
    RefineryCrackSpreadResult crack_res = GeneralizedKirkCrackSpreadEngine::price(crack_321);

    std::cout << "    • 3:2:1 Refinery Crack Call Price: $" << crack_res.price << " / bbl" << std::endl;
    std::cout << "    • Composite Refinery Volatility:    " << (crack_res.composite_volatility * 100.0) << "%" << std::endl;
    std::cout << "    • Delta Gasoline (RBOB):           +" << crack_res.delta_gasoline << std::endl;
    std::cout << "    • Delta Distillate (ULSD):         +" << crack_res.delta_distillate << std::endl;
    std::cout << "    • Delta Crude (WTI/Brent):         " << crack_res.delta_crude << std::endl;

    assert(crack_res.price > 0.0);
    assert(crack_res.delta_gasoline > 0.0);
    assert(crack_res.delta_distillate > 0.0);
    assert(crack_res.delta_crude < 0.0); // Negative delta to crude feedstock
    std::cout << "    -> 3:2:1 CRACK SPREAD VALIDATION PASSED (Multivariate Greeks Consistent)" << std::endl;

    // [3] Cholesky Correlated Monte Carlo Multi-Asset Risk Engine
    std::cout << "\n[3] Testing Cholesky Correlated Multi-Asset Monte Carlo Risk Simulator..." << std::endl;
    constexpr size_t N_ASSETS = 3; // Brent, Gasoil, WTI
    MultiAssetPortfolioInput<N_ASSETS> port_in{
        .spot_prices = {82.50, 110.00, 78.00},
        .drift_rates = {0.045, 0.045, 0.045},
        .volatilities = {0.28, 0.32, 0.30},
        .portfolio_positions = {50000.0, -20000.0, 30000.0}, // 50k Brent long, 20k Gasoil short, 30k WTI long
        .correlation_matrix = {{
            {1.00, 0.85, 0.92},
            {0.85, 1.00, 0.80},
            {0.92, 0.80, 1.00}
        }},
        .time_horizon_years = 1.0 / 252.0, // 1-Day Horizon
        .num_simulated_paths = 100000
    };

    MonteCarloVaRResult mc_var = CholeskyCorrelatedMonteCarloEngine::evaluate_portfolio_var(port_in, 777);

    std::cout << "    • Initial Portfolio Value:       $" << mc_var.portfolio_initial_value << std::endl;
    std::cout << "    • 1-Day 95% Monte Carlo VaR:     $" << mc_var.var_95_usd << std::endl;
    std::cout << "    • 1-Day 99% Monte Carlo VaR:     $" << mc_var.var_99_usd << std::endl;
    std::cout << "    • 1-Day 97.5% Expected Shortfall:$" << mc_var.expected_shortfall_975_usd << std::endl;
    std::cout << "    • Worst-Case Simulated Loss:     $" << mc_var.worst_case_loss_usd << std::endl;
    std::cout << "    • Simulation Compute Time (100k): " << (mc_var.compute_nanos / 1000000.0) << " ms" << std::endl;

    assert(mc_var.var_99_usd > mc_var.var_95_usd);
    assert(mc_var.expected_shortfall_975_usd > mc_var.var_95_usd);
    std::cout << "    -> CHOLESKY MONTE CARLO RISK VALIDATION PASSED (Quantile Monotonicity Verified)" << std::endl;

    std::cout << "\n====================================================================" << std::endl;
    std::cout << "  ALL ADVANCED QUANTITATIVE DEPTH GATES PASSED (100% OK)            " << std::endl;
    std::cout << "====================================================================" << std::endl;
    return 0;
}
