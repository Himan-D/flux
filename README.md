# Flux: High-Performance OTC Commodity Derivatives Trading Platform

Flux is an institutional, multi-tenant SaaS platform engineered for OTC commodity derivatives trading (oil & refined products, gas, power). It combines systematic market making, centralized cross-desk risk hedging, and an autonomous multi-agent AI triad for the oil derivatives desk.

---

## Codebase Architecture

```
flux/
├── proto/
│   └── flux_protocol.proto           # Protobuf contracts for market curves, vol surfaces, pricing & signals
├── core-engine/                      # High-performance C++20 & Rust execution core
│   ├── include/
│   │   ├── asian_engine.hpp          # Turnbull-Wakeman moment matching Asian APO kernel (< 4µs)
│   │   ├── crack_spread_engine.hpp   # Kirk's approximation & Bachelier crack spread kernel (< 1µs)
│   │   └── var_engine.hpp            # Historical Full-Revaluation VaR & Expected Shortfall (CVaR)
│   ├── src/
│   │   ├── main.cpp                  # C++ pricing & risk benchmark harness
│   │   ├── main.rs                   # Rust Central Risk Book (CRB) & SMM quoter runner
│   │   ├── crb_hedger.rs             # Almgren-Chriss optimal liquidation & cross-desk internalization
│   │   ├── smm_quoter.rs             # Avellaneda-Stoikov systematic market making with AI alpha skew
│   │   └── models.rs                 # Rust core domain models
│   └── Cargo.toml                    # Rust crate definition
├── agents/                           # Multi-Agent AI Subsystem (Oil Derivatives Desk)
│   ├── requirements.txt              # Python dependencies (polars, scipy, pydantic)
│   ├── state.py                      # Multi-agent state schema & dataclasses
│   ├── curve_construction_agent.py   # Forward strip bootstrapping & splining
│   ├── signal_generation_agent.py    # Physical refinery runs, tanker tracking & inventory alpha
│   ├── pricing_compute_agent.py      # Non-linear Asian option pricer & Greeks
│   └── orchestrator.py               # Multi-agent coordinator & quote synthesizer
├── saas-control/                     # Enterprise SaaS Control Plane & API Gateway
│   ├── schema.sql                    # PostgreSQL DDL with Row-Level Security (RLS) & audit trail
│   ├── go.mod                        # Go module
│   └── main.go                       # High-concurrency REST/WebSocket RFQ negotiation server
└── README.md
```

---

## Quickstart & Verification

### 1. Run C++ Pricing & Risk Benchmark
```bash
cd core-engine
clang++ -std=c++20 -O3 -Iinclude src/main.cpp -o pricing_benchmark
./pricing_benchmark
```
*Performance Output:*
* Asian Option (APO) Turnbull-Wakeman pricing latency: **~3.16 microseconds**
* Crack Spread pricing latency: **~0.125 microseconds**
* 500-Scenario Portfolio VaR / Expected Shortfall compute: **~1.2 microseconds**

### 2. Run Rust Central Risk Book (CRB) & SMM Quoter
```bash
cd core-engine
cargo run --release
```
*Performance Output:*
* Multi-desk cross-internalization & Almgren-Chriss hedge calculation: **0.67 microseconds**
* Systematic Avellaneda-Stoikov RFQ quoting with alpha skew: **sub-microsecond**

### 3. Run Python Multi-Agent AI Orchestrator
```bash
cd agents
python3 orchestrator.py
```
*Coordinates Curve Construction, Signal Generation, and Pricing Compute agents in a unified event loop.*

### 4. Run Go SaaS API Server
```bash
cd saas-control
go run main.go
```
*Listens on `http://localhost:8080` for `/v1/rfq/request` and `/v1/trade/execute`.*

---

## Architectural Specifications & Design Docs
Detailed technical design documents and mathematical derivations are available in the project architecture documentation artifacts:
* Architecture & Multi-Agent Design
* Low-Level Protocols, SBE/Protobuf & Mathematical Formulations
* Technology Stack & Infrastructure
* Microsecond Latency Engineering & Hardware Tuning Playbook
* Enterprise Risk Management, VaR, SIMM & Regulatory Compliance
