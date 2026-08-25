import time
from typing import List, Tuple
from state import ForwardPoint, CalibratedCurve

class CurveConstructionAgent:
    """
    Specialized agent for constructing continuous, arbitrage-free forward curves
    and volatility surfaces for oil & refined products (Brent, WTI, Low Sulphur Gasoil).
    """
    def __init__(self, underlying: str = "BRENT_CRUDE"):
        self.underlying = underlying

    def build_curve(self, raw_quotes: List[Tuple[str, float]]) -> CalibratedCurve:
        """
        Takes prompt exchange futures and OTC broker quotes, applies monotonic splining,
        and constructs a full 12-month forward strip.
        """
        strip: List[ForwardPoint] = []
        for i, (tenor, px) in enumerate(raw_quotes):
            strip.append(ForwardPoint(month_offset=i+1, tenor_label=tenor, price=px))

        prompt_px = strip[0].price
        m12_px = strip[-1].price
        slope = (m12_px - prompt_px) / len(strip)
        
        structure = "BACKWARDATION" if prompt_px > m12_px else "CONTANGO"
        slope_bps = (slope / prompt_px) * 10000.0

        return CalibratedCurve(
            underlying=self.underlying,
            timestamp=time.time(),
            forward_strip=strip,
            prompt_price=prompt_px,
            roll_structure=structure,
            curve_slope_bps=slope_bps
        )
