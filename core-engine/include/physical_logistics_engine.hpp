#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace flux::logistics {

struct VesselFixture {
    std::string vessel_name;
    std::string charter_party_type; // "SHELLVOY5", "ASBATANKVOY"
    double parcel_size_bbl;
    double laytime_allowed_hours;
    double demurrage_rate_per_day_usd;
    double actual_laytime_used_hours;
};

struct DemurrageResult {
    double demurrage_incurred_usd;
    double excess_hours;
    bool is_in_demurrage;
};

struct CrudeBlendingInput {
    double light_sweet_bbl; // e.g. WTI (API: 40.0, Sulfur: 0.2%)
    double light_sweet_api;
    double light_sweet_sulfur_pct;

    double heavy_sour_bbl;  // e.g. Maya / Western Canadian Select (API: 21.0, Sulfur: 3.5%)
    double heavy_sour_api;
    double heavy_sour_sulfur_pct;
};

struct BlendedCrudeResult {
    double total_volume_bbl;
    double blended_api_gravity;
    double blended_sulfur_pct;
    double target_grade_quality_penalty_usd;
    uint64_t compute_nanos;
};

class PhysicalLogisticsEngine {
public:
    /// Computes demurrage penalty based on standard maritime charter contracts
    static DemurrageResult compute_demurrage(const VesselFixture& fixture) {
        double excess_hours = std::max(0.0, fixture.actual_laytime_used_hours - fixture.laytime_allowed_hours);
        double demurrage_usd = (excess_hours / 24.0) * fixture.demurrage_rate_per_day_usd;

        return DemurrageResult{
            .demurrage_incurred_usd = demurrage_usd,
            .excess_hours = excess_hours,
            .is_in_demurrage = excess_hours > 0.0
        };
    }

    /// Blending optimization: Computes non-linear specific gravity and sulfur blend
    static BlendedCrudeResult compute_crude_blend(const CrudeBlendingInput& blend) {
        auto start = std::chrono::high_resolution_clock::now();

        double v1 = blend.light_sweet_bbl;
        double v2 = blend.heavy_sour_bbl;
        double total_vol = v1 + v2;

        if (total_vol <= 0.0) {
            return BlendedCrudeResult{};
        }

        // Specific Gravity: SG = 141.5 / (API + 131.5)
        double sg1 = 141.5 / (blend.light_sweet_api + 131.5);
        double sg2 = 141.5 / (blend.heavy_sour_api + 131.5);

        // Blended Specific Gravity is volume-weighted
        double blended_sg = (v1 * sg1 + v2 * sg2) / total_vol;
        // Invert SG to get blended API
        double blended_api = (141.5 / blended_sg) - 131.5;

        // Sulfur content is weight-weighted: Mass = Volume * SG
        double mass1 = v1 * sg1;
        double mass2 = v2 * sg2;
        double blended_sulfur = (mass1 * blend.light_sweet_sulfur_pct + mass2 * blend.heavy_sour_sulfur_pct) / (mass1 + mass2);

        // Quality penalty if sulfur exceeds 0.5% (IMO 2020 limit)
        double penalty = 0.0;
        if (blended_sulfur > 0.50) {
            penalty = (blended_sulfur - 0.50) * 2.50 * total_vol; // $2.50/bbl per % sulfur excess
        }

        auto end = std::chrono::high_resolution_clock::now();
        uint64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return BlendedCrudeResult{
            .total_volume_bbl = total_vol,
            .blended_api_gravity = blended_api,
            .blended_sulfur_pct = blended_sulfur,
            .target_grade_quality_penalty_usd = penalty,
            .compute_nanos = nanos
        };
    }
};

} // namespace flux::logistics
