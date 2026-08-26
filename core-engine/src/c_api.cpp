#include "../include/c_api.h"
#include "../include/asian_engine.hpp"
#include "../include/sabr_engine.hpp"
#include "../include/crack_spread_engine.hpp"
#include <cstring>

using namespace flux::pricing;

extern "C" {

int flux_price_turnbull_wakeman(const CAsianInput* in, CPricingResult* out) {
    if (!in || !out) return -1;

    AsianPricerInput cpp_in{};
    cpp_in.is_call = in->is_call;
    cpp_in.strike = in->strike;
    cpp_in.risk_free_rate = in->risk_free_rate;
    cpp_in.time_to_maturity = in->time_to_maturity;
    cpp_in.total_fixings_count = in->total_fixings_count;
    cpp_in.realized_count = in->realized_count;
    cpp_in.remaining_count = in->remaining_count;

    for (size_t i = 0; i < 32; ++i) {
        cpp_in.forward_strip[i] = in->forward_strip[i];
        cpp_in.time_points[i] = in->time_points[i];
        cpp_in.volatilities[i] = in->volatilities[i];
        cpp_in.realized_fixings[i] = in->realized_fixings[i];
    }

    PricingResult res = TurnbullWakemanAsianKernel::price(cpp_in);
    out->price = res.price;
    out->delta = res.delta;
    out->gamma = res.gamma;
    out->vega = res.vega;
    out->theta = res.theta;
    out->latency_nanos = res.latency_nanos;

    return 0;
}

int flux_price_sabr(const CSABRInput* in, CSABRResult* out) {
    if (!in || !out) return -1;

    SABRParameters p{
        .alpha = in->alpha,
        .beta = in->beta,
        .rho = in->rho,
        .nu = in->nu,
        .forward_price = in->forward_price,
        .strike = in->strike,
        .time_to_maturity = in->time_to_maturity
    };

    SABRResult res = SABREngine::compute_implied_vol(p);
    out->implied_volatility = res.implied_volatility;
    out->dvol_dstrike = res.dvol_dstrike;
    out->dvol_dalpha = res.dvol_dalpha;
    out->compute_nanos = res.compute_nanos;

    return 0;
}

int flux_price_321_crack_spread(const CRefineryCrackInput* in, CRefineryCrackResult* out) {
    if (!in || !out) return -1;

    RefineryCrackSpreadInput cpp_in{
        .is_call = in->is_call,
        .weight_gasoline = in->weight_gasoline,
        .weight_distillate = in->weight_distillate,
        .weight_crude = in->weight_crude,
        .forward_gasoline = in->forward_gasoline,
        .forward_distillate = in->forward_distillate,
        .forward_crude = in->forward_crude,
        .vol_gasoline = in->vol_gasoline,
        .vol_distillate = in->vol_distillate,
        .vol_crude = in->vol_crude,
        .corr_gas_dist = in->corr_gas_dist,
        .corr_gas_crude = in->corr_gas_crude,
        .corr_dist_crude = in->corr_dist_crude,
        .strike_spread = in->strike_spread,
        .risk_free_rate = in->risk_free_rate,
        .time_to_maturity = in->time_to_maturity
    };

    RefineryCrackSpreadResult res = GeneralizedKirkCrackSpreadEngine::price(cpp_in);
    out->price = res.price;
    out->delta_gasoline = res.delta_gasoline;
    out->delta_distillate = res.delta_distillate;
    out->delta_crude = res.delta_crude;
    out->composite_volatility = res.composite_volatility;
    out->cross_vega = res.cross_vega;
    out->latency_nanos = res.latency_nanos;

    return 0;
}

}
