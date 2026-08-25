from dataclasses import dataclass, field
from typing import List, Dict, Optional
import time

@dataclass
class ForwardPoint:
    month_offset: int
    tenor_label: str
    price: float

@dataclass
class CalibratedCurve:
    underlying: str
    timestamp: float
    forward_strip: List[ForwardPoint]
    prompt_price: float
    roll_structure: str # "BACKWARDATION" or "CONTANGO"
    curve_slope_bps: float

@dataclass
class MarketSignal:
    timestamp: float
    directional_bias: float # [-1.0, +1.0]
    prompt_roll_pressure: float
    refinery_margin_indicator: str
    tanker_congestion_score: float # [0.0 to 1.0]
    recommended_quote_skew_bps: float
    reasoning: str

@dataclass
class OptionQuoteResult:
    instrument: str
    strike: float
    call_fair_value: float
    put_fair_value: float
    delta: float
    gamma: float
    vega: float
    theta: float
    compute_time_ms: float

@dataclass
class MultiAgentDeskState:
    desk_id: str
    latest_curve: Optional[CalibratedCurve] = None
    latest_signal: Optional[MarketSignal] = None
    active_quotes: Dict[str, OptionQuoteResult] = field(default_factory=dict)
    event_log: List[str] = field(default_factory=list)
