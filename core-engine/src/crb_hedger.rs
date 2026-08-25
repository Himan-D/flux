use std::collections::HashMap;
use std::time::Instant;
use crate::models::{BenchmarkAsset, OptimalHedgeSlice, RiskSensitivity};

pub struct CentralRiskBook {
    pub risk_aversion: f64,    // Lambda parameter
    pub temp_impact_coeff: f64, // Eta
    pub market_vols: HashMap<BenchmarkAsset, f64>,
}

impl CentralRiskBook {
    pub fn new(risk_aversion: f64, temp_impact_coeff: f64) -> Self {
        let mut market_vols = HashMap::new();
        market_vols.insert(BenchmarkAsset::IceBrent, 0.28);
        market_vols.insert(BenchmarkAsset::NymexWti, 0.30);
        market_vols.insert(BenchmarkAsset::IceGasoil, 0.32);
        market_vols.insert(BenchmarkAsset::NymexRbob, 0.35);

        Self {
            risk_aversion,
            temp_impact_coeff,
            market_vols,
        }
    }

    /// Internalizes cross-desk positions and calculates optimal Almgren-Chriss hedge orders
    pub fn compute_hedges(&self, exposures: &[RiskSensitivity], horizon_seconds: f64) -> (Vec<OptimalHedgeSlice>, f64) {
        let start = Instant::now();
        let mut aggregated_delta: HashMap<BenchmarkAsset, f64> = HashMap::new();

        // 1. Cross-Desk Internalization Netting
        for exp in exposures {
            let entry = aggregated_delta.entry(exp.asset).or_insert(0.0);
            *entry += exp.delta_quantity;
        }

        // 2. Almgren-Chriss Optimal Hedging Trajectory for Net Residuals
        let mut hedge_slices = Vec::new();
        for (asset, net_delta) in aggregated_delta {
            // If net position within internal tolerance band (500 bbls), skip exchange order
            if net_delta.abs() < 500.0 {
                continue;
            }

            let vol = self.market_vols.get(&asset).copied().unwrap_or(0.30);
            // Kappa = sqrt( (lambda * sigma^2) / eta )
            let kappa = ((self.risk_aversion * vol * vol) / self.temp_impact_coeff).sqrt();
            let hedge_qty = -net_delta;

            hedge_slices.push(OptimalHedgeSlice {
                asset,
                target_order_qty: hedge_qty,
                urgency_parameter_kappa: kappa,
                horizon_seconds,
            });
        }

        let elapsed_micros = start.elapsed().as_secs_f64() * 1_000_000.0;
        (hedge_slices, elapsed_micros)
    }
}
