use std::collections::{HashMap, HashSet};
use std::time::{SystemTime, UNIX_EPOCH};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum OrderSide {
    Buy,
    Sell,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum OrderState {
    NewSubmitted,
    PendingAck,
    Acked,
    PartiallyFilled,
    Filled,
    PendingCancel,
    Cancelled,
    Rejected,
    Expired,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OrderRequest {
    pub cl_ord_id: String,
    pub desk_id: String,
    pub symbol: String,
    pub side: OrderSide,
    pub price: f64,
    pub quantity: f64,
    pub max_notional_limit: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ExecutionReport {
    pub exec_id: String,
    pub cl_ord_id: String,
    pub order_id: String,
    pub state: OrderState,
    pub cum_qty: f64,
    pub leaves_qty: f64,
    pub avg_price: f64,
    pub text: String,
    pub timestamp_ns: u128,
}

pub struct PreTradeRiskLimits {
    pub max_single_order_notional: f64,
    pub max_price_collar_pct: f64, // e.g. 0.05 = 5% max deviation from mid
}

pub struct OrderStateMachine {
    pub risk_limits: PreTradeRiskLimits,
    pub known_cl_ord_ids: HashSet<String>,
    pub active_orders: HashMap<String, (OrderRequest, OrderState, f64)>, // (Req, State, CumQty)
    pub exec_seq: u64,
}

impl OrderStateMachine {
    pub fn new(max_notional: f64, max_price_collar_pct: f64) -> Self {
        Self {
            risk_limits: PreTradeRiskLimits {
                max_single_order_notional: max_notional,
                max_price_collar_pct,
            },
            known_cl_ord_ids: HashSet::new(),
            active_orders: HashMap::new(),
            exec_seq: 0,
        }
    }

    fn now_ns() -> u128 {
        SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos()
    }

    /// Submits a new order through the pre-trade risk filter and transitions to PendingAck or Rejected
    pub fn submit_order(&mut self, req: OrderRequest, market_mid: f64) -> ExecutionReport {
        self.exec_seq += 1;
        let exec_id = format!("EXEC-{:08}", self.exec_seq);
        let order_id = format!("ORD-{:08}", self.exec_seq);

        // 1. Idempotency Check (Duplicate ClOrdID)
        if self.known_cl_ord_ids.contains(&req.cl_ord_id) {
            return ExecutionReport {
                exec_id,
                cl_ord_id: req.cl_ord_id,
                order_id: "".to_string(),
                state: OrderState::Rejected,
                cum_qty: 0.0,
                leaves_qty: 0.0,
                avg_price: 0.0,
                text: "REJECTED: Duplicate ClOrdID (Idempotency Violation)".to_string(),
                timestamp_ns: Self::now_ns(),
            };
        }

        // 2. Pre-Trade Risk: Max Notional Check
        let notional = req.price * req.quantity;
        if notional > self.risk_limits.max_single_order_notional {
            return ExecutionReport {
                exec_id,
                cl_ord_id: req.cl_ord_id,
                order_id: "".to_string(),
                state: OrderState::Rejected,
                cum_qty: 0.0,
                leaves_qty: req.quantity,
                avg_price: 0.0,
                text: format!("REJECTED: Order notional ${:.2} exceeds limit ${:.2}", notional, self.risk_limits.max_single_order_notional),
                timestamp_ns: Self::now_ns(),
            };
        }

        // 3. Pre-Trade Risk: Price Collar Check
        let price_diff_pct = (req.price - market_mid).abs() / market_mid;
        if price_diff_pct > self.risk_limits.max_price_collar_pct {
            return ExecutionReport {
                exec_id,
                cl_ord_id: req.cl_ord_id,
                order_id: "".to_string(),
                state: OrderState::Rejected,
                cum_qty: 0.0,
                leaves_qty: req.quantity,
                avg_price: 0.0,
                text: format!("REJECTED: Price collar violation ({:.2}% deviation from mid)", price_diff_pct * 100.0),
                timestamp_ns: Self::now_ns(),
            };
        }

        // Order accepted into state machine
        self.known_cl_ord_ids.insert(req.cl_ord_id.clone());
        self.active_orders.insert(req.cl_ord_id.clone(), (req.clone(), OrderState::Acked, 0.0));

        ExecutionReport {
            exec_id,
            cl_ord_id: req.cl_ord_id,
            order_id,
            state: OrderState::Acked,
            cum_qty: 0.0,
            leaves_qty: req.quantity,
            avg_price: req.price,
            text: "ORDER_ACKED: Pre-trade risk passed, active on book".to_string(),
            timestamp_ns: Self::now_ns(),
        }
    }

    /// Processes an execution fill event (Partial or Full)
    pub fn process_fill(&mut self, cl_ord_id: &str, fill_qty: f64, fill_price: f64) -> Result<ExecutionReport, String> {
        let (req, state, cum_qty) = match self.active_orders.get_mut(cl_ord_id) {
            Some(entry) => entry,
            None => return Err(format!("Order not found for ClOrdID: {}", cl_ord_id)),
        };

        if *state == OrderState::Filled || *state == OrderState::Cancelled || *state == OrderState::Rejected {
            return Err(format!("Cannot fill order in terminal state: {:?}", state));
        }

        self.exec_seq += 1;
        let exec_id = format!("EXEC-{:08}", self.exec_seq);

        *cum_qty += fill_qty;
        let leaves_qty = (req.quantity - *cum_qty).max(0.0);

        let new_state = if leaves_qty == 0.0 {
            OrderState::Filled
        } else {
            OrderState::PartiallyFilled
        };
        *state = new_state;

        Ok(ExecutionReport {
            exec_id,
            cl_ord_id: cl_ord_id.to_string(),
            order_id: format!("ORD-{}", cl_ord_id),
            state: new_state,
            cum_qty: *cum_qty,
            leaves_qty,
            avg_price: fill_price,
            text: format!("FILL: {:.0} bbl @ ${:.2}", fill_qty, fill_price),
            timestamp_ns: Self::now_ns(),
        })
    }

    /// Cancels an active order
    pub fn cancel_order(&mut self, cl_ord_id: &str) -> Result<ExecutionReport, String> {
        let (req, state, cum_qty) = match self.active_orders.get_mut(cl_ord_id) {
            Some(entry) => entry,
            None => return Err(format!("Order not found for ClOrdID: {}", cl_ord_id)),
        };

        if *state == OrderState::Filled || *state == OrderState::Cancelled {
            return Err(format!("Cannot cancel order in state: {:?}", state));
        }

        *state = OrderState::Cancelled;
        self.exec_seq += 1;

        Ok(ExecutionReport {
            exec_id: format!("EXEC-{:08}", self.exec_seq),
            cl_ord_id: cl_ord_id.to_string(),
            order_id: format!("ORD-{}", cl_ord_id),
            state: OrderState::Cancelled,
            cum_qty: *cum_qty,
            leaves_qty: 0.0,
            avg_price: req.price,
            text: "ORDER_CANCELLED: Removed from market".to_string(),
            timestamp_ns: Self::now_ns(),
        })
    }
}
