#ifndef FLUX_C_API_H
#define FLUX_C_API_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 1. C ABI Asian Option Structs
typedef struct {
    bool is_call;
    double strike;
    double risk_free_rate;
    double time_to_maturity;
    size_t total_fixings_count;
    size_t realized_count;
    size_t remaining_count;
    double forward_strip[32];
    double time_points[32];
    double volatilities[32];
    double realized_fixings[32];
} CAsianInput;

typedef struct {
    double price;
    double delta;
    double gamma;
    double vega;
    double theta;
    uint64_t latency_nanos;
} CPricingResult;

// 2. C ABI SABR Volatility Structs
typedef struct {
    double alpha;
    double beta;
    double rho;
    double nu;
    double forward_price;
    double strike;
    double time_to_maturity;
} CSABRInput;

typedef struct {
    double implied_volatility;
    double dvol_dstrike;
    double dvol_dalpha;
    uint64_t compute_nanos;
} CSABRResult;

// 3. C ABI 3:2:1 Crack Spread Structs
typedef struct {
    bool is_call;
    double weight_gasoline;
    double weight_distillate;
    double weight_crude;
    double forward_gasoline;
    double forward_distillate;
    double forward_crude;
    double vol_gasoline;
    double vol_distillate;
    double vol_crude;
    double corr_gas_dist;
    double corr_gas_crude;
    double corr_dist_crude;
    double strike_spread;
    double risk_free_rate;
    double time_to_maturity;
} CRefineryCrackInput;

typedef struct {
    double price;
    double delta_gasoline;
    double delta_distillate;
    double delta_crude;
    double composite_volatility;
    double cross_vega;
    uint64_t latency_nanos;
} CRefineryCrackResult;

// Exported C ABI Functions
int flux_price_turnbull_wakeman(const CAsianInput* in, CPricingResult* out);
int flux_price_sabr(const CSABRInput* in, CSABRResult* out);
int flux_price_321_crack_spread(const CRefineryCrackInput* in, CRefineryCrackResult* out);

#ifdef __cplusplus
}
#endif

#endif // FLUX_C_API_H
