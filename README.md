# Flux: Experimental High-Performance Quantitative Engine for OTC Commodity Derivatives

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![CI Status](https://img.shields.io/badge/CI-Passing-brightgreen.svg)](.github/workflows/ci.yml)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20_AVX--512-blue.svg)](core-engine/include/)
[![Rust](https://img.shields.io/badge/Language-Rust_1.75%2B-orange.svg)](core-engine/src/)
[![Go](https://img.shields.io/badge/Language-Go_1.22%2B-00ADD8.svg)](cli/)
[![Python](https://img.shields.io/badge/Language-Python_3.11%2B-yellow.svg)](agents/)

Flux is an experimental, open-source research and trading infrastructure prototype focused on Over-the-Counter (OTC) commodity derivatives (crude oil, refined products, and crack spreads). It combines C++20 analytical pricing kernels, Rust systematic market-making models, a Go terminal CLI, and an advisory Python multi-agent system.

---

## 1. Implementation Status & Truth Matrix

To ensure clear boundaries between verified mathematical kernels, working prototypes, and experimental simulations, the following table details the implementation state of each subsystem:

| Subsystem | Source Location | Implementation Depth | Status & Validation |
| :--- | :--- | :--- | :--- |
| **Asian Option (APO) Kernel** | `core-engine/include/asian_engine.hpp` | C++20 Turnbull-Wakeman moment matching with branchless Cody rational minimax CDF. Stack-allocated fixed arrays (`alignas(64)`). | **Verified Core Math** (Validated within 0.17% of 200k-path Monte Carlo reference in `test_reference_validation.cpp`). |
| **Crack Spread Option Kernel** | `core-engine/include/crack_spread_engine.hpp` | Kirk's bivariate lognormal analytical approximation with cross-commodity correlation and analytical Greeks. | **Verified Core Math** (Analytical formulas for 1:1 spread; generalized 3:2:1 refinery ratio in progress). |
| **SMM Quoter** | `core-engine/src/smm_quoter.rs` | Rust implementation of Avellaneda-Stoikov inventory reservation pricing with volatility-adjusted half-spread. | **Algorithmic Prototype** (Uses linear heuristic alpha skew rather than solving the Poisson intensity ODE). |
| **Central Risk Book (CRB)** | `core-engine/src/crb_hedger.rs` | Cross-desk factor delta aggregation and Almgren-Chriss urgency parameter ($\kappa$) calculation. | **Algorithmic Prototype** (Computes optimal static slice; dynamic multi-period fill simulator in progress). |
| **Historical VaR & CVaR** | `core-engine/include/var_engine.hpp` | Quantile extraction for 95%, 97.5%, and 99% Historical VaR and Expected Shortfall ($E[L \mid L > \text{VaR}]$). | **Analytical Utility** (Operates on pre-generated PnL arrays; full covariance scenario generator in progress). |
| **Bilateral XVA** | `core-engine/include/xva_engine.hpp` | 1D numerical integration of CVA, DVA, and FVA over credit hazard rate curves and funding spreads. | **Deterministic Numerical Model** (Riemann sum over EPE/ENE; not a multi-asset stochastic path simulation). |
| **Physical CTRM Blending** | `core-engine/include/physical_logistics_engine.hpp` | ASTM D1298 specific gravity inversion, volume-weighted SG blending, and mass-weighted sulfur conservation. | **Analytical Model** (2-stream blend formula with IMO 2020 0.50% check; LP simplex solver planned). |
| **Consensus Sequencer** | `core-engine/src/aeron_cluster_sequencer.rs` | In-memory loop simulating 3-node Raft quorum and sequence numbering. | **In-Memory Simulation** (Does not link to external `libaeron.a` or write to real `/dev/shm` IPC channels). |
| **Multi-Agent AI Desk** | `agents/*.py` | Modular Python state machine for forward curve splining, demurrage auditing, and advisory alpha skewing. | **Heuristic Rule Engine** (Advisory only; uses deterministic rules on synthetic inputs, not live broker/satellite APIs). |
| **SaaS Gateway & CLI** | `saas-control/` & `cli/` | 16-way sharded mutex state maps, HMAC-SHA256 JWT auth, Prometheus `/metrics`, and terminal shell. | **Working Gateway Prototype** (In-memory sharded storage; PostgreSQL RLS DDL provided in `schema.sql`). |

---

## 2. Rigorous Benchmark Methodology & Environment

The benchmark methodology uses **batch timing** (10,000 iterations per batch across 1,000,000 total runs) to eliminate timer syscall overhead and hardware clock tick quantization on macOS ARM64.

### Test Environment
* **Hardware**: Apple Silicon ARM64 (Darwin 25.3.0)
* **Compilers**: Clang++ (C++20, `-std=c++20 -O3 -Wall -Wextra`), Rust 1.75+ (`rustc --release`)
* **Measurement**: Batch measurement (10,000 iters/batch), pre-warmed L1 instruction/data caches, stack-allocated inputs.

### Measured Kernel Latency Distributions

| Kernel / Operation | Runtime | Throughput (ops/sec) | Batch Mean (p50) | Batch p90 | Batch p99 |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **Turnbull-Wakeman Asian APO** | C++20 (Cody Poly) | **1,840,173** | **528.67 ns** | **573.77 ns** | **953.10 ns** |
| **Kirk Crack Spread Option** | C++20 Analytical | **31,390,275** | **31.98 ns** | **33.12 ns** | **34.58 ns** |
| **SMM Avellaneda-Stoikov Quoter** | Rust Lock-Free | **17,512,521** | **52.98 ns** | **75.05 ns** | **90.90 ns** |
| **CRB Factor Netting & Urgency**| Rust Almgren-Chriss | **8,831,289** | **108.04 ns** | **124.50 ns** | **148.42 ns** |
| **ISDA SIMM Margin Call Audit** | Rust CSA Evaluator | **24,788,933** | **40.07 ns** | **48.20 ns** | **54.63 ns** |
| **Sharded SaaS Gateway (50k reqs)**| Go (50 workers) | **131,164** | **< 1 ms** | **1.2 ms** | **1.8 ms** |

---

## 3. Mathematical Reference Validation

Flux includes automated mathematical validation against independent references in [`core-engine/tests/test_reference_validation.cpp`](core-engine/tests/test_reference_validation.cpp):

```bash
# Run reference tolerance validation
clang++ -std=c++20 -O3 -Icore-engine/include core-engine/tests/test_reference_validation.cpp -o test_ref && ./test_ref
```

```
[1] Turnbull-Wakeman vs 200,000-Path Monte Carlo Reference:
    • Turnbull-Wakeman Analytical: $2.98265
    • Monte Carlo Reference:        $2.98780
    • Relative Error:               0.17244% (PASS: Within Monte Carlo standard error)

[2] Analytical Delta vs Finite-Difference Gradient:
    • Analytical Delta:             0.53855
    • Finite-Difference Delta:      0.53999
    • Absolute Difference:          0.00144 (PASS: Within gradient tolerance)
```

---

## 4. Architecture & Safety Boundaries

```
[EXTERNAL INPUTS] (Platts / ICE / Telemetry)
       │
       ▼
[ADVISORY LAYER (Python)]
  • Curve Splining & Advisory Alpha Skew (+8.75 bps)
       │ (Advisory Signal Only — Never Touches Execution Directly)
       ▼
[DETERMINISTIC PRE-TRADE RISK & QUOTING (Rust / C++)]
  • Avellaneda-Stoikov Reservation Pricing
  • Central Risk Book Factor Netting (Almgren-Chriss)
  • Strict Position, Credit & CSA Margin Limits
       │
       ▼
[EXECUTION & AUDIT LAYER (Go / PostgreSQL RLS)]
  • SHA-256 Hash-Chained Audit Ledger
  • Prometheus Telemetry
```

---

## 5. Quickstart

### Build All Binaries
```bash
make build
```

### Run All Validation Test Suites
```bash
make test
```

### Run the Benchmark Suite
```bash
make benchmark
```

---

## 6. License
Licensed under the [Apache License, Version 2.0](LICENSE).
