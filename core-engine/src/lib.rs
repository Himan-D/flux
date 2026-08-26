pub mod models;
pub mod crb_hedger;
pub mod smm_quoter;
pub mod collateral_simm_manager;
pub mod aeron_cluster_sequencer;
pub mod order_state_machine;

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
        let mut seq = AeronClusterSequencer::new_3node_cluster();
        let ev1 = seq.sequence_and_commit("payload_1");
        let ev2 = seq.sequence_and_commit("payload_2");

        assert_eq!(ev1.sequence_number, 1);
        assert_eq!(ev2.sequence_number, 2);
        assert_eq!(ev1.quorum_acks, 2, "3-node cluster must require 2 ACKs for quorum");
    }

    #[test]
    fn test_order_state_machine_lifecycle() {
        use order_state_machine::{OrderStateMachine, OrderRequest, OrderSide, OrderState};

        let mut osm = OrderStateMachine::new(1_000_000.0, 0.05); // $1M limit, 5% collar

        // 1. Submit normal order
        let req = OrderRequest {
            cl_ord_id: "ORD-TEST-001".to_string(),
            desk_id: "DESK_CRUDE_LON".to_string(),
            symbol: "BRENT".to_string(),
            side: OrderSide::Buy,
            price: 82.50,
            quantity: 5000.0,
            max_notional_limit: 1_000_000.0,
        };
        let ack = osm.submit_order(req, 82.50);
        assert_eq!(ack.state, OrderState::Acked);

        // 2. Reject duplicate ClOrdID (Idempotency)
        let dup_req = OrderRequest {
            cl_ord_id: "ORD-TEST-001".to_string(),
            desk_id: "DESK_CRUDE_LON".to_string(),
            symbol: "BRENT".to_string(),
            side: OrderSide::Buy,
            price: 82.50,
            quantity: 5000.0,
            max_notional_limit: 1_000_000.0,
        };
        let dup_ack = osm.submit_order(dup_req, 82.50);
        assert_eq!(dup_ack.state, OrderState::Rejected);
        assert!(dup_ack.text.contains("Duplicate ClOrdID"));

        // 3. Reject price collar violation
        let bad_price_req = OrderRequest {
            cl_ord_id: "ORD-TEST-002".to_string(),
            desk_id: "DESK_CRUDE_LON".to_string(),
            symbol: "BRENT".to_string(),
            side: OrderSide::Buy,
            price: 100.00, // 21% away from mid 82.50
            quantity: 1000.0,
            max_notional_limit: 1_000_000.0,
        };
        let collar_ack = osm.submit_order(bad_price_req, 82.50);
        assert_eq!(collar_ack.state, OrderState::Rejected);
        assert!(collar_ack.text.contains("Price collar violation"));

        // 4. Partial Fill & Full Fill
        let fill1 = osm.process_fill("ORD-TEST-001", 2000.0, 82.50).unwrap();
        assert_eq!(fill1.state, OrderState::PartiallyFilled);
        assert_eq!(fill1.leaves_qty, 3000.0);

        let fill2 = osm.process_fill("ORD-TEST-001", 3000.0, 82.50).unwrap();
        assert_eq!(fill2.state, OrderState::Filled);
        assert_eq!(fill2.leaves_qty, 0.0);
    }
}
