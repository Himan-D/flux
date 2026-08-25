pub mod models;
pub mod crb_hedger;
pub mod smm_quoter;
pub mod collateral_simm_manager;
pub mod aeron_cluster_sequencer;

#[cfg(test)]
mod tests {
    use super::*;
    use models::{BenchmarkAsset, RiskSensitivity};
    use crb_hedger::CentralRiskBook;
    use smm_quoter::SystematicMarketMaker;
    use collateral_simm_manager::{CollateralSIMMManager, CounterpartyCSAAgreement};
    use aeron_cluster_sequencer::AeronClusterSequencer;

    #[test]
    fn test_crb_internalization_netting() {
        let crb = CentralRiskBook::new(0.00005, 0.0001);
        let exposures = vec![
            RiskSensitivity { desk_id: "D1".to_string(), asset: BenchmarkAsset::IceBrent, delta_quantity: 50_000.0, vega: 10_000.0 },
            RiskSensitivity { desk_id: "D2".to_string(), asset: BenchmarkAsset::IceBrent, delta_quantity: -50_000.0, vega: 5_000.0 },
        ];

        let (hedges, _) = crb.compute_hedges(&exposures, 300.0);
        // Net delta is 0, so no external hedges should be generated
        assert_eq!(hedges.len(), 0, "Perfectly offsetting positions should yield 0 external hedges");
    }

    #[test]
    fn test_smm_quote_skew() {
        let smm = SystematicMarketMaker::new(0.000002, 3.5);
        let (quote_neutral, _) = smm.generate_quote(BenchmarkAsset::IceBrent, 80.0, 0.0, 0.25, 1.0/252.0, 0.0);
        let (quote_bullish, _) = smm.generate_quote(BenchmarkAsset::IceBrent, 80.0, 0.0, 0.25, 1.0/252.0, 0.8);

        assert!(quote_bullish.reservation_price > quote_neutral.reservation_price, "Bullish AI signal must increase reservation price");
        assert!(quote_bullish.bid_price > quote_neutral.bid_price, "Bullish AI signal must lift bid price");
    }

    #[test]
    fn test_collateral_margin_call_threshold() {
        let csa = CounterpartyCSAAgreement {
            counterparty_id: "CPTY_1".to_string(),
            threshold_usd: 1_000_000.0,
            minimum_transfer_amount_usd: 100_000.0,
            current_collateral_posted_usd: 0.0,
            isda_simm_initial_margin_required_usd: 200_000.0,
        };

        // MTM below threshold
        let action_low = CollateralSIMMManager::evaluate_margin_call(&csa, 500_000.0);
        assert_eq!(action_low.variation_margin_call_usd, 0.0);
        assert!(action_low.requires_transfer, "SIMM IM exceeds MTA, requiring transfer");

        // MTM above threshold
        let action_high = CollateralSIMMManager::evaluate_margin_call(&csa, 2_500_000.0);
        assert_eq!(action_high.variation_margin_call_usd, 1_500_000.0);
        assert_eq!(action_high.total_margin_due_usd, 1_700_000.0);
    }

    #[test]
    fn test_aeron_sequencer_monotonicity() {
        let mut sequencer = AeronClusterSequencer::new_3node_cluster();
        let ev1 = sequencer.sequence_and_commit("ORDER_1");
        let ev2 = sequencer.sequence_and_commit("ORDER_2");

        assert_eq!(ev1.sequence_number, 1);
        assert_eq!(ev2.sequence_number, 2);
        assert!(ev2.sequence_number > ev1.sequence_number);
        assert_eq!(ev1.quorum_acks, 2);
    }
}
