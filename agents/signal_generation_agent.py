import time
from state import MarketSignal, CalibratedCurve

class SignalGenerationAgent:
    """
    Synthesizes tanker tracking, refinery utilization data, Cushing/ARA storage levels,
    and prompt contract roll pressures to generate actionable quantitative alpha signals.
    """
    def __init__(self, desk_name: str = "OIL_DERIVATIVES"):
        self.desk_name = desk_name

    def evaluate_signals(
        self,
        curve: CalibratedCurve,
        tanker_inflow_kbpd: float,
        refinery_run_utilization_pct: float,
        cushing_inventory_delta_mbbl: float
    ) -> MarketSignal:
        
        # Rule 1: High refinery run utilization (> 92%) + low inventory draw = Bullish product demand
        demand_score = 0.0
        if refinery_run_utilization_pct > 90.0:
            demand_score += 0.4
        if cushing_inventory_delta_mbbl < -1.5:
            demand_score += 0.35 # Strong inventory draw

        # Rule 2: Tanker congestion / floating storage
        congestion_score = min(1.0, max(0.0, tanker_inflow_kbpd / 2500.0))
        if congestion_score > 0.7:
            demand_score -= 0.3 # Supply bottleneck / floating build

        # Rule 3: Curve Structure Momentum
        if curve.roll_structure == "BACKWARDATION":
            demand_score += 0.25 # Physical prompt tightness

        bias = max(-1.0, min(1.0, demand_score))
        skew_bps = bias * 12.5 # Up to 12.5 bps skew adjustment

        reasoning = (
            f"Refinery utilization at {refinery_run_utilization_pct:.1f}%, "
            f"Cushing draw {cushing_inventory_delta_mbbl} MBbl, "
            f"Curve in strong {curve.roll_structure} ({curve.curve_slope_bps:.1f} bps slope)."
        )

        return MarketSignal(
            timestamp=time.time(),
            directional_bias=bias,
            prompt_roll_pressure=0.85 if curve.roll_structure == "BACKWARDATION" else -0.5,
            refinery_margin_indicator="EXPANDING" if refinery_run_utilization_pct > 88.0 else "COMPRESSING",
            tanker_congestion_score=congestion_score,
            recommended_quote_skew_bps=skew_bps,
            reasoning=reasoning
        )
