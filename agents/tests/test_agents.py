import pytest
import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from curve_construction_agent import CurveConstructionAgent
from signal_generation_agent import SignalGenerationAgent
from physical_logistics_agent import PhysicalLogisticsAgent
from pricing_compute_agent import PricingComputeAgent

def test_curve_construction():
    agent = CurveConstructionAgent("BRENT_CRUDE")
    raw_quotes = [("M01", 80.0), ("M02", 78.0), ("M03", 76.0)]
    curve = agent.build_curve(raw_quotes)
    
    assert curve.roll_structure == "BACKWARDATION"
    assert curve.prompt_price == 80.0
    assert len(curve.forward_strip) == 3

def test_signal_generation_bullish():
    curve_agent = CurveConstructionAgent("BRENT_CRUDE")
    curve = curve_agent.build_curve([("M01", 82.0), ("M02", 80.0)])
    
    signal_agent = SignalGenerationAgent("OIL_DESK")
    signal = signal_agent.evaluate_signals(
        curve=curve,
        tanker_inflow_kbpd=1000.0,
        refinery_run_utilization_pct=95.0,
        cushing_inventory_delta_mbbl=-3.0
    )
    
    assert signal.directional_bias > 0.5, "Strong refinery demand and inventory draw must be bullish"
    assert signal.recommended_quote_skew_bps > 0.0

def test_physical_logistics_demurrage():
    logistics = PhysicalLogisticsAgent()
    voyage = logistics.track_vessel_fixture(
        vessel_name="TEST_VESSEL",
        imo="IMO-1234567",
        origin="Jubail",
        destination="Singapore",
        grade="Gasoil",
        volume_bbl=500000.0,
        allowed_laytime_hrs=48.0,
        actual_laytime_hrs=72.0, # 24 hrs excess
        demurrage_rate_per_day=48000.0
    )
    
    assert voyage.laytime_status == "IN_DEMURRAGE"
    assert voyage.estimated_demurrage_usd == 48000.0

def test_pricing_asian_option():
    curve_agent = CurveConstructionAgent("BRENT_CRUDE")
    curve = curve_agent.build_curve([("M01", 80.0), ("M02", 80.0)])
    
    pricing = PricingComputeAgent()
    res = pricing.price_asian_option(
        curve=curve,
        strike=80.0,
        time_to_maturity=0.25,
        volatility=0.30,
        realized_fixings=[80.0, 80.0],
        total_fixings=10,
        risk_free_rate=0.05
    )
    
    assert res.call_fair_value > 0.0
    assert 0.0 < res.delta < 1.0
    assert res.vega > 0.0

if __name__ == "__main__":
    test_curve_construction()
    test_signal_generation_bullish()
    test_physical_logistics_demurrage()
    test_pricing_asian_option()
    print("All Python agent unit tests passed!")
