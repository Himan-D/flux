use std::time::Instant;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CounterpartyCSAAgreement {
    pub counterparty_id: String,
    pub threshold_usd: f64,
    pub minimum_transfer_amount_usd: f64, // MTA
    pub current_collateral_posted_usd: f64,
    pub isda_simm_initial_margin_required_usd: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MarginCallAction {
    pub counterparty_id: String,
    pub variation_margin_call_usd: f64,
    pub total_margin_due_usd: f64,
    pub requires_transfer: bool,
    pub calculation_latency_nanos: u128,
}

pub struct CollateralSIMMManager;

impl CollateralSIMMManager {
    /// Computes real-time dynamic margin calls under ISDA SIMM + VM Credit Support Annex (CSA)
    pub fn evaluate_margin_call(
        csa: &CounterpartyCSAAgreement,
        current_mtm_exposure_usd: f64,
    ) -> MarginCallAction {
        let start = Instant::now();

        // Variation Margin requirement = max(0, MTM - Threshold)
        let uncollateralized_mtm = current_mtm_exposure_usd - csa.threshold_usd;
        let required_vm = if uncollateralized_mtm > 0.0 { uncollateralized_mtm } else { 0.0 };

        // Total required collateral = Required VM + ISDA SIMM IM
        let total_required = required_vm + csa.isda_simm_initial_margin_required_usd;
        let margin_shortfall = total_required - csa.current_collateral_posted_usd;

        // Check if shortfall exceeds Minimum Transfer Amount (MTA)
        let requires_transfer = margin_shortfall >= csa.minimum_transfer_amount_usd;
        let total_due = if requires_transfer { margin_shortfall } else { 0.0 };

        let nanos = start.elapsed().as_nanos();

        MarginCallAction {
            counterparty_id: csa.counterparty_id.clone(),
            variation_margin_call_usd: required_vm,
            total_margin_due_usd: total_due,
            requires_transfer,
            calculation_latency_nanos: nanos,
        }
    }
}
