use std::time::Instant;
use crate::models::{BenchmarkAsset, QuoteProposal};

pub struct SystematicMarketMaker {
    pub inventory_penalty_gamma: f64,
    pub base_half_spread_bps: f64,
}

impl SystematicMarketMaker {
    pub fn new(gamma: f64, base_half_spread_bps: f64) -> Self {
        Self {
            inventory_penalty_gamma: gamma,
            base_half_spread_bps,
        }
    }

    /// Avellaneda-Stoikov systematic market making with inventory skew and AI signal injection
    pub fn generate_quote(
        &self,
        asset: BenchmarkAsset,
        mid_price: f64,
        inventory_q: f64,
        annualized_vol: f64,
        time_horizon_years: f64,
        ai_signal_bias: f64, // [-1.0, +1.0]
    ) -> (QuoteProposal, f64) {
        let start = Instant::now();

        // 1. Reservation Price: r(s, q, t) = s - q * gamma * sigma^2 * (T - t) + AI_bias
        let variance = annualized_vol * annualized_vol;
        let inventory_skew = inventory_q * self.inventory_penalty_gamma * variance * time_horizon_years;
        let signal_skew = ai_signal_bias * 0.15; // 15 cents max signal shift
        
        let reservation_price = mid_price - inventory_skew + signal_skew;

        // 2. Dynamic Spread based on volatility
        let half_spread = (self.base_half_spread_bps / 10_000.0) * mid_price * (1.0 + annualized_vol);

        let bid_price = reservation_price - half_spread;
        let ask_price = reservation_price + half_spread;
        let skew_applied_bps = ((reservation_price - mid_price) / mid_price) * 10_000.0;

        let elapsed_micros = start.elapsed().as_secs_f64() * 1_000_000.0;

        let proposal = QuoteProposal {
            asset,
            reservation_price,
            bid_price,
            ask_price,
            half_spread,
            skew_applied_bps,
        };

        (proposal, elapsed_micros)
    }
}
