import time
from dataclasses import dataclass
from typing import List, Dict

@dataclass
class VesselVoyageState:
    vessel_name: str
    imo_number: str
    origin_port: str
    destination_port: str
    cargo_grade: str
    volume_bbl: float
    eta_timestamp: float
    laytime_status: str # "ON_SCHEDULE", "AT_RISK", "IN_DEMURRAGE"
    estimated_demurrage_usd: float

class PhysicalLogisticsAgent:
    """
    Tracks maritime vessel fixtures, pipeline batch nominations (e.g. Colonial, Capline),
    and port terminal demurrage risk to forecast physical delivery friction.
    """
    def __init__(self):
        self.active_voyages: Dict[str, VesselVoyageState] = {}

    def track_vessel_fixture(
        self,
        vessel_name: str,
        imo: str,
        origin: str,
        destination: str,
        grade: str,
        volume_bbl: float,
        allowed_laytime_hrs: float,
        actual_laytime_hrs: float,
        demurrage_rate_per_day: float = 65000.0
    ) -> VesselVoyageState:
        excess_hrs = max(0.0, actual_laytime_hrs - allowed_laytime_hrs)
        demurrage = (excess_hrs / 24.0) * demurrage_rate_per_day
        status = "IN_DEMURRAGE" if excess_hrs > 0 else ("AT_RISK" if actual_laytime_hrs > allowed_laytime_hrs * 0.8 else "ON_SCHEDULE")

        state = VesselVoyageState(
            vessel_name=vessel_name,
            imo_number=imo,
            origin_port=origin,
            destination_port=destination,
            cargo_grade=grade,
            volume_bbl=volume_bbl,
            eta_timestamp=time.time() + 86400 * 3, # 3 days out
            laytime_status=status,
            estimated_demurrage_usd=demurrage
        )
        self.active_voyages[imo] = state
        return state
