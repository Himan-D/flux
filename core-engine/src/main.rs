use std::time::Instant;
use flux_core::crb_hedger::CentralRiskBook;
use flux_core::smm_quoter::SystematicMarketMaker;
use flux_core::collateral_simm_manager::{CollateralSIMMManager, CounterpartyCSAAgreement};
use flux_core::models::{BenchmarkAsset, RiskSensitivity};

fn main() {
    println!("====================================================================");
    println!("  FLUX RUST BENCHMARK HARNESS (BATCH-TIMED & SPECIFIED)             ");
    println!("====================================================================");
    println!("  Environment Specifications:");
    println!("  • Toolchain:       Rust 1.75+ (rustc --release, target-cpu=native)");
    println!("  • Measurement:     Batch Timing (10,000 iters/batch to eliminate");
    println!("                     timer syscall distortion and 41.67ns clock ticks)");
    println!("====================================================================\n");

    const TOTAL_RUNS: usize = 1_000_000;
    const BATCH_SIZE: usize = 10_000;
    const NUM_BATCHES: usize = TOTAL_RUNS / BATCH_SIZE;

    // 1. Systematic Market Maker Quote Proposals
    println!("[1] Benchmarking SMM Quote Proposals (Avellaneda-Stoikov + Skew) (1,000,000 runs)...");
    let smm = SystematicMarketMaker::new(0.0001, 2.5);
    let mut batch_latencies_ns = Vec::with_capacity(NUM_BATCHES);

    let start_total_smm = Instant::now();
    for b in 0..NUM_BATCHES {
        let t_start = Instant::now();
        for i in 0..BATCH_SIZE {
            let inv = ((b * BATCH_SIZE + i) % 20000) as f64 - 10000.0;
            let (_quote, _) = smm.generate_quote(
                BenchmarkAsset::IceBrent,
                82.50,
                inv,
                0.28,
                0.25,
                0.35,
            );
        }
        let batch_ns = t_start.elapsed().as_nanos() as f64 / BATCH_SIZE as f64;
        batch_latencies_ns.push(batch_ns);
    }
    let total_smm_ms = start_total_smm.elapsed().as_secs_f64() * 1000.0;
    let smm_ops_sec = (TOTAL_RUNS as f64 / total_smm_ms) * 1000.0;

    batch_latencies_ns.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let p50_smm = batch_latencies_ns[NUM_BATCHES * 50 / 100];
    let p90_smm = batch_latencies_ns[NUM_BATCHES * 90 / 100];
    let p99_smm = batch_latencies_ns[NUM_BATCHES * 99 / 100];

    println!("    -> Total Wall Time:   {:.2} ms", total_smm_ms);
    println!("    -> Average Speed:     {:.2} Million quotes / sec ({:.0} ops/s)", smm_ops_sec / 1_000_000.0, smm_ops_sec);
    println!("    -> Batch Mean (p50):  {:.2} ns per quote", p50_smm);
    println!("    -> Batch p90:         {:.2} ns", p90_smm);
    println!("    -> Batch p99:         {:.2} ns\n", p99_smm);

    // 2. Central Risk Book Factor Netting
    println!("[2] Benchmarking Central Risk Book Multi-Desk Factor Netting (100,000 runs)...");
    let crb = CentralRiskBook::new(0.0001, 0.00005);
    let exposures = vec![
        RiskSensitivity { desk_id: "CRUDE_LON".to_string(), asset: BenchmarkAsset::IceBrent, delta_quantity: 100000.0, vega: 45000.0 },
        RiskSensitivity { desk_id: "DIST_GEN".to_string(), asset: BenchmarkAsset::IceGasoil, delta_quantity: -60000.0, vega: 20000.0 },
        RiskSensitivity { desk_id: "LIGHT_HOU".to_string(), asset: BenchmarkAsset::NymexWti, delta_quantity: 40000.0, vega: 18000.0 },
        RiskSensitivity { desk_id: "CRUDE_HOU".to_string(), asset: BenchmarkAsset::IceBrent, delta_quantity: -75000.0, vega: 30000.0 },
    ];

    const CRB_RUNS: usize = 100_000;
    const CRB_BATCH: usize = 1_000;
    const CRB_NUM_BATCHES: usize = CRB_RUNS / CRB_BATCH;
    let mut crb_batch_latencies = Vec::with_capacity(CRB_NUM_BATCHES);

    let start_crb = Instant::now();
    for _ in 0..CRB_NUM_BATCHES {
        let t_start = Instant::now();
        for _ in 0..CRB_BATCH {
            let (_slices, _) = crb.compute_hedges(&exposures, 300.0);
        }
        let batch_ns = t_start.elapsed().as_nanos() as f64 / CRB_BATCH as f64;
        crb_batch_latencies.push(batch_ns);
    }
    let total_crb_ms = start_crb.elapsed().as_secs_f64() * 1000.0;
    let crb_ops_sec = (CRB_RUNS as f64 / total_crb_ms) * 1000.0;

    crb_batch_latencies.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let p50_crb = crb_batch_latencies[CRB_NUM_BATCHES * 50 / 100];
    let p99_crb = crb_batch_latencies[CRB_NUM_BATCHES * 99 / 100];

    println!("    -> Total Wall Time:   {:.2} ms", total_crb_ms);
    println!("    -> Average Speed:     {:.2} Million rebalances / sec ({:.0} ops/s)", crb_ops_sec / 1_000_000.0, crb_ops_sec);
    println!("    -> Batch Mean (p50):  {:.2} ns per rebalance", p50_crb);
    println!("    -> Batch p99:         {:.2} ns\n", p99_crb);

    // 3. Dynamic ISDA SIMM Margin Call Evaluator
    println!("[3] Benchmarking Dynamic ISDA SIMM Margin Call Calculations (1,000,000 runs)...");
    let csa = CounterpartyCSAAgreement {
        counterparty_id: "CPTY_GLENCORE_ENERGY".to_string(),
        threshold_usd: 5_000_000.0,
        minimum_transfer_amount_usd: 500_000.0,
        current_collateral_posted_usd: 6_200_000.0,
        isda_simm_initial_margin_required_usd: 3_800_000.0,
    };
    let mut simm_batch_latencies = Vec::with_capacity(NUM_BATCHES);
    let start_simm = Instant::now();

    for b in 0..NUM_BATCHES {
        let t_start = Instant::now();
        for i in 0..BATCH_SIZE {
            let mtm = 5_000_000.0 + ((b * BATCH_SIZE + i) % 10000) as f64 * 1000.0;
            let _action = CollateralSIMMManager::evaluate_margin_call(&csa, mtm);
        }
        let batch_ns = t_start.elapsed().as_nanos() as f64 / BATCH_SIZE as f64;
        simm_batch_latencies.push(batch_ns);
    }
    let total_simm_ms = start_simm.elapsed().as_secs_f64() * 1000.0;
    let simm_ops_sec = (TOTAL_RUNS as f64 / total_simm_ms) * 1000.0;

    simm_batch_latencies.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let p50_simm = simm_batch_latencies[NUM_BATCHES * 50 / 100];
    let p99_simm = simm_batch_latencies[NUM_BATCHES * 99 / 100];

    println!("    -> Total Wall Time:   {:.2} ms", total_simm_ms);
    println!("    -> Average Speed:     {:.2} Million evaluations / sec ({:.0} ops/s)", simm_ops_sec / 1_000_000.0, simm_ops_sec);
    println!("    -> Batch Mean (p50):  {:.2} ns per evaluation", p50_simm);
    println!("    -> Batch p99:         {:.2} ns\n", p99_simm);

    println!("====================================================================");
    println!("  ALL RUST BATCH BENCHMARKS COMPLETED                               ");
    println!("====================================================================");
}
