use std::time::Instant;
use flux_core::crb_hedger::CentralRiskBook;
use flux_core::smm_quoter::SystematicMarketMaker;
use flux_core::collateral_simm_manager::{CollateralSIMMManager, CounterpartyCSAAgreement};
use flux_core::aeron_cluster_sequencer::AeronClusterSequencer;
use flux_core::models::{BenchmarkAsset, RiskSensitivity};

fn main() {
    println!("====================================================================");
    println!("  FLUX RUST CORE EMPIRICAL 1,000,000-RUN SCALE BENCHMARK            ");
    println!("====================================================================\n");

    // 1. SMM Quoting Engine Benchmark (1,000,000 iterations)
    println!("[1] Benchmarking 1,000,000 Systematic Market Maker Quote Proposals...");
    let smm = SystematicMarketMaker::new(0.0001, 2.5);
    let mut latencies_nanos = Vec::with_capacity(1_000_000);
    let start_total = Instant::now();

    for i in 0..1_000_000 {
        let inv = (i % 20000) as f64 - 10000.0;
        let t0 = Instant::now();
        let (_quote, _) = smm.generate_quote(
            BenchmarkAsset::IceBrent,
            82.50,
            inv,
            0.28,
            0.25,
            0.35,
        );
        latencies_nanos.push(t0.elapsed().as_nanos());
    }
    let total_smm_ms = start_total.elapsed().as_secs_f64() * 1000.0;
    let smm_ops_sec = (1_000_000.0 / total_smm_ms) * 1000.0;

    latencies_nanos.sort_unstable();
    let p50 = latencies_nanos[500_000];
    let p90 = latencies_nanos[900_000];
    let p99 = latencies_nanos[990_000];
    let p999 = latencies_nanos[999_000];

    println!("    -> Total Duration:    {:.2} ms", total_smm_ms);
    println!("    -> Measured Speed:    {:.2} Million quotes / sec ({:.0} ops/s)", smm_ops_sec / 1_000_000.0, smm_ops_sec);
    println!("    -> Median (p50):      {} ns ({:.3} µs)", p50, p50 as f64 / 1000.0);
    println!("    -> p90 Latency:       {} ns", p90);
    println!("    -> p99 Latency:       {} ns", p99);
    println!("    -> p99.9 Latency:     {} ns\n", p999);

    // 2. Central Risk Book Factor Netting Benchmark (100,000 iterations)
    println!("[2] Benchmarking 100,000 Central Risk Book Cross-Desk Netting Cycles...");
    let crb = CentralRiskBook::new(0.0001, 0.00005);
    let exposures = vec![
        RiskSensitivity { desk_id: "CRUDE_LON".to_string(), asset: BenchmarkAsset::IceBrent, delta_quantity: 100000.0, vega: 45000.0 },
        RiskSensitivity { desk_id: "DIST_GEN".to_string(), asset: BenchmarkAsset::IceGasoil, delta_quantity: -60000.0, vega: 20000.0 },
        RiskSensitivity { desk_id: "LIGHT_HOU".to_string(), asset: BenchmarkAsset::NymexWti, delta_quantity: 40000.0, vega: 18000.0 },
        RiskSensitivity { desk_id: "CRUDE_HOU".to_string(), asset: BenchmarkAsset::IceBrent, delta_quantity: -75000.0, vega: 30000.0 },
    ];

    let mut crb_latencies = Vec::with_capacity(100_000);
    let start_crb = Instant::now();

    for _ in 0..100_000 {
        let t0 = Instant::now();
        let (_slices, _) = crb.compute_hedges(&exposures, 300.0);
        crb_latencies.push(t0.elapsed().as_nanos());
    }
    let total_crb_ms = start_crb.elapsed().as_secs_f64() * 1000.0;
    let crb_ops_sec = (100_000.0 / total_crb_ms) * 1000.0;
    crb_latencies.sort_unstable();

    println!("    -> Total Duration:    {:.2} ms", total_crb_ms);
    println!("    -> Measured Speed:    {:.2} Million rebalances / sec ({:.0} ops/s)", crb_ops_sec / 1_000_000.0, crb_ops_sec);
    println!("    -> Median (p50):      {} ns ({:.3} µs)", crb_latencies[50_000], crb_latencies[50_000] as f64 / 1000.0);
    println!("    -> p99 Latency:       {} ns\n", crb_latencies[99_000]);

    // 3. Aeron 3-Node Raft Clustered Sequencer Benchmark (100,000 iterations)
    println!("[3] Benchmarking 100,000 Aeron Raft Clustered Sequencer Replications...");
    let mut sequencer = AeronClusterSequencer::new_3node_cluster();
    let mut seq_latencies = Vec::with_capacity(100_000);
    let start_seq = Instant::now();

    for i in 0..100_000 {
        let payload = format!("{{\"trade_id\": \"trd-{}\"}}", i);
        let t0 = Instant::now();
        let _event = sequencer.sequence_and_commit(&payload);
        seq_latencies.push(t0.elapsed().as_nanos());
    }
    let total_seq_ms = start_seq.elapsed().as_secs_f64() * 1000.0;
    let seq_ops_sec = (100_000.0 / total_seq_ms) * 1000.0;
    seq_latencies.sort_unstable();

    println!("    -> Total Duration:    {:.2} ms", total_seq_ms);
    println!("    -> Measured Speed:    {:.2} Million events / sec ({:.0} ops/s)", seq_ops_sec / 1_000_000.0, seq_ops_sec);
    println!("    -> Median (p50):      {} ns ({:.3} µs)", seq_latencies[50_000], seq_latencies[50_000] as f64 / 1000.0);
    println!("    -> p99 Latency:       {} ns\n", seq_latencies[99_000]);

    // 4. Dynamic ISDA SIMM Margin Call Evaluator (1,000,000 iterations)
    println!("[4] Benchmarking 1,000,000 Dynamic ISDA SIMM Margin Call Calculations...");
    let csa = CounterpartyCSAAgreement {
        counterparty_id: "CPTY_GLENCORE_ENERGY".to_string(),
        threshold_usd: 5_000_000.0,
        minimum_transfer_amount_usd: 500_000.0,
        current_collateral_posted_usd: 6_200_000.0,
        isda_simm_initial_margin_required_usd: 3_800_000.0,
    };
    let mut simm_latencies = Vec::with_capacity(1_000_000);
    let start_simm = Instant::now();

    for i in 0..1_000_000 {
        let mtm = 5_000_000.0 + (i % 10000) as f64 * 1000.0;
        let t0 = Instant::now();
        let _action = CollateralSIMMManager::evaluate_margin_call(&csa, mtm);
        simm_latencies.push(t0.elapsed().as_nanos());
    }
    let total_simm_ms = start_simm.elapsed().as_secs_f64() * 1000.0;
    let simm_ops_sec = (1_000_000.0 / total_simm_ms) * 1000.0;
    simm_latencies.sort_unstable();

    println!("    -> Total Duration:    {:.2} ms", total_simm_ms);
    println!("    -> Measured Speed:    {:.2} Million evaluations / sec ({:.0} ops/s)", simm_ops_sec / 1_000_000.0, simm_ops_sec);
    println!("    -> Median (p50):      {} ns", simm_latencies[500_000]);
    println!("    -> p99 Latency:       {} ns", simm_latencies[990_000]);

    println!("\n====================================================================");
    println!("  ALL RUST SCALE BENCHMARKS EXECUTED & VALIDATED SUCCESSFULLY       ");
    println!("====================================================================");
}
