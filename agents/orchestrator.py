import time
import json
from state import MultiAgentDeskState
from curve_construction_agent import CurveConstructionAgent
from signal_generation_agent import SignalGenerationAgent
from pricing_compute_agent import PricingComputeAgent
from physical_logistics_agent import PhysicalLogisticsAgent

class OilDeskOrchestrator:
    """
    Coordinates the multi-agent AI system for the Oil Derivatives trading desk:
    1. Curve Construction Agent   -> Calibrates forward strip & vol surface
    2. Signal Generation Agent    -> Evaluates physical & order flow telemetry
    3. Physical Logistics Agent   -> Monitors vessel voyages, laytime & demurrage
    4. Pricing Compute Agent      -> Prices complex Asian APO structures & Greeks
    """
    def __init__(self, desk_id: str = "DESK_OIL_DERIVATIVES_LONDON"):
        self.state = MultiAgentDeskState(desk_id=desk_id)
        self.curve_agent = CurveConstructionAgent("BRENT_CRUDE")
        self.signal_agent = SignalGenerationAgent("OIL_DERIVATIVES")
        self.logistics_agent = PhysicalLogisticsAgent()
        self.pricing_agent = PricingComputeAgent()

    def process_market_event(self):
        print("=========================================================")
        print("  FLUX MULTI-AGENT AI DESK ORCHESTRATOR                  ")
        print(f"  Desk ID: {self.state.desk_id}                          ")
        print("=========================================================\n")

        # 1. Curve Construction Agent
        print("[1] Curve Construction Agent: Ingesting prompt exchange strip...")
        raw_quotes = [
            ("M01 (Oct-26)", 82.50),
            ("M02 (Nov-26)", 81.80),
            ("M03 (Dec-26)", 81.15),
            ("M04 (Jan-27)", 80.60),
            ("M05 (Feb-27)", 80.10),
            ("M06 (Mar-27)", 79.70),
            ("M12 (Sep-27)", 77.90),
        ]
        curve = self.curve_agent.build_curve(raw_quotes)
        self.state.latest_curve = curve
        print(f"    -> Calibrated {curve.underlying} Strip ({len(curve.forward_strip)} points)")
        print(f"    -> Structure: {curve.roll_structure} (Slope: {curve.curve_slope_bps:.2f} bps/mo)")
        print(f"    -> Prompt Benchmark: ${curve.prompt_price:.2f} / bbl\n")

        # 2. Physical Logistics Agent
        print("[2] Physical Logistics Agent: Auditing active vessel parcels & laytime...")
        voyage = self.logistics_agent.track_vessel_fixture(
            vessel_name="DHT HAWK (VLCC)",
            imo="IMO-9812345",
            origin="Ras Tanura",
            destination="Rotterdam",
            grade="Arab Light",
            volume_bbl=2000000.0,
            allowed_laytime_hrs=72.0,
            actual_laytime_hrs=96.0,
            demurrage_rate_per_day=65000.0
        )
        print(f"    -> Tracked Vessel: {voyage.vessel_name} ({voyage.volume_bbl:,.0f} bbl {voyage.cargo_grade})")
        print(f"    -> Voyage Route: {voyage.origin_port} -> {voyage.destination_port}")
        print(f"    -> Laytime Status: {voyage.laytime_status} (Accrued Demurrage: ${voyage.estimated_demurrage_usd:,.2f})\n")

        # 3. Signal Generation Agent
        print("[3] Signal Generation Agent: Analyzing physical telemetry & tanker delays...")
        signal = self.signal_agent.evaluate_signals(
            curve=curve,
            tanker_inflow_kbpd=1800.0,
            refinery_run_utilization_pct=93.4,
            cushing_inventory_delta_mbbl=-2.1
        )
        self.state.latest_signal = signal
        print(f"    -> Directional Alpha Score: {signal.directional_bias:+.2f}")
        print(f"    -> Recommended SMM Skew: {signal.recommended_quote_skew_bps:+.2f} bps")
        print(f"    -> Agent Reasoning: \"{signal.reasoning}\"\n")

        # 4. Pricing Compute Agent
        print("[4] Pricing Compute Agent: Evaluating inbound OTC Asian Option RFQ...")
        quote = self.pricing_agent.price_asian_option(
            curve=curve,
            strike=81.50,
            time_to_maturity=0.25,
            volatility=0.28,
            realized_fixings=[82.10, 82.40, 82.80, 83.00],
            total_fixings=21,
            risk_free_rate=0.045
        )
        self.state.active_quotes["RFQ_BRENT_APO_81.50"] = quote
        print(f"    -> Instrument: {quote.instrument} (Strike: ${quote.strike:.2f})")
        print(f"    -> Fair Value Call: ${quote.call_fair_value:.4f} / bbl")
        print(f"    -> Delta: {quote.delta:.4f} | Gamma: {quote.gamma:.4f} | Vega: {quote.vega:.4f}")
        print(f"    -> Pricing Compute Latency: {quote.compute_time_ms:.3f} ms\n")

        # 5. Desk Synthesis & Final Quoting Decision
        base_bid = quote.call_fair_value - 0.05
        base_ask = quote.call_fair_value + 0.05
        skew_shift = (signal.recommended_quote_skew_bps / 10000.0) * curve.prompt_price
        
        skewed_bid = base_bid + skew_shift
        skewed_ask = base_ask + skew_shift

        print("[5] Desk Synthesis & SMM Quote Publishing:")
        print(f"    -> Skewed Streaming Bid: ${skewed_bid:.4f} (Shift: +${skew_shift:.4f})")
        print(f"    -> Skewed Streaming Ask: ${skewed_ask:.4f}")
        print(f"    -> Internal Spread: ${(skewed_ask - skewed_bid):.4f}")
        print("\n=========================================================")
        print("  MULTI-AGENT EXECUTION CYCLE COMPLETED                  ")
        print("=========================================================")

if __name__ == "__main__":
    orchestrator = OilDeskOrchestrator()
    orchestrator.process_market_event()
