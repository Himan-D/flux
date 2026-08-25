import math
import time
from typing import List
from state import OptionQuoteResult, CalibratedCurve

class PricingComputeAgent:
    """
    Computes real-time analytical pricing and Greeks for Asian Average Price Options (APOs)
    and Crack Spreads, factoring in realized historical fixings and future forward strips.
    """
    def __init__(self):
        pass

    @staticmethod
    def _norm_cdf(x: float) -> float:
        return 0.5 * (1.0 + math.erf(x / math.sqrt(2.0)))

    def price_asian_option(
        self,
        curve: CalibratedCurve,
        strike: float,
        time_to_maturity: float,
        volatility: float,
        realized_fixings: List[float],
        total_fixings: int,
        risk_free_rate: float = 0.045,
        is_call: bool = True
    ) -> OptionQuoteResult:
        start = time.perf_counter()
        
        m = len(realized_fixings)
        N = total_fixings
        k = N - m
        
        realized_sum = sum(realized_fixings) if m > 0 else 0.0
        remaining_weight = k / N
        adjusted_strike = strike - (realized_sum / N if m > 0 else 0.0)

        # Average forward from calibrated strip
        strip_slice = curve.forward_strip[:k]
        F_avg = sum(p.price for p in strip_slice) / len(strip_slice) if strip_slice else curve.prompt_price

        # Turnbull-Wakeman moment matched variance
        eff_vol = volatility * math.sqrt((2.0 * k + 1.0) / (3.0 * N))
        sigma_sqrt_T = eff_vol * math.sqrt(time_to_maturity)
        
        F_eff = remaining_weight * F_avg
        K_eff = adjusted_strike
        df = math.exp(-risk_free_rate * time_to_maturity)

        if K_eff <= 0:
            call_px = df * (F_eff - K_eff)
            put_px = 0.0
            delta = df * remaining_weight
        else:
            d1 = (math.log(F_eff / K_eff) + 0.5 * (eff_vol ** 2) * time_to_maturity) / sigma_sqrt_T
            d2 = d1 - sigma_sqrt_T
            call_px = df * (F_eff * self._norm_cdf(d1) - K_eff * self._norm_cdf(d2))
            put_px = df * (K_eff * self._norm_cdf(-d2) - F_eff * self._norm_cdf(-d1))
            delta = df * remaining_weight * self._norm_cdf(d1)

        gamma = (df / (F_eff * sigma_sqrt_T * math.sqrt(2 * math.pi))) * math.exp(-0.5 * (d1**2)) if K_eff > 0 else 0.0
        vega = df * F_eff * math.sqrt(time_to_maturity) * (1.0 / math.sqrt(2 * math.pi)) * math.exp(-0.5 * (d1**2)) if K_eff > 0 else 0.0
        theta = - (df * F_eff * eff_vol) / (2 * math.sqrt(time_to_maturity) * math.sqrt(2 * math.pi)) if K_eff > 0 else 0.0

        elapsed_ms = (time.perf_counter() - start) * 1000.0

        return OptionQuoteResult(
            instrument=f"{curve.underlying}_ASIAN_APO",
            strike=strike,
            call_fair_value=call_px,
            put_fair_value=put_px,
            delta=delta,
            gamma=gamma,
            vega=vega,
            theta=theta,
            compute_time_ms=elapsed_ms
        )
