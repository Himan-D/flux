use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum BenchmarkAsset {
    IceBrent,
    NymexWti,
    IceGasoil,
    NymexRbob,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RiskSensitivity {
    pub desk_id: String,
    pub asset: BenchmarkAsset,
    pub delta_quantity: f64, // Positive = Long, Negative = Short (bbl / MT equiv)
    pub vega: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OptimalHedgeSlice {
    pub asset: BenchmarkAsset,
    pub target_order_qty: f64,
    pub urgency_parameter_kappa: f64,
    pub horizon_seconds: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QuoteProposal {
    pub asset: BenchmarkAsset,
    pub reservation_price: f64,
    pub bid_price: f64,
    pub ask_price: f64,
    pub half_spread: f64,
    pub skew_applied_bps: f64,
}
