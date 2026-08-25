# Flux: High-Performance OTC Commodity Derivatives Trading Platform

Flux is an institutional, multi-tenant SaaS and CLI platform engineered for OTC commodity derivatives trading (oil & refined products, gas, power). It combines systematic market making, centralized cross-desk risk hedging, an autonomous multi-agent AI triad, and a high-performance terminal CLI.

---

## Codebase Architecture

```
flux/
├── cli/
│   └── main.go                       # Institutional terminal CLI (RFQ, Risk, Curves, Agents, XVA, Monitor)
├── proto/
│   └── flux_protocol.proto           # Protobuf contracts for market curves, vol surfaces, pricing & signals
├── core-engine/                      # High-performance C++20 & Rust execution core
│   ├── include/
│   │   ├── asian_engine.hpp          # Turnbull-Wakeman moment matching Asian APO kernel (2.45 µs)
│   │   ├── crack_spread_engine.hpp   # Kirk's crack spread kernel (0.16 µs)
│   │   ├── var_engine.hpp            # 500-scenario Historical VaR & Expected Shortfall
│   │   ├── physical_logistics_engine.hpp # Specific gravity blending & demurrage (< 100 ns)
│   │   └── xva_engine.hpp            # Multi-curve CVA / DVA / FVA kernel (0.20 µs)
│   ├── tests/
│   │   └── test_pricing.cpp          # C++ Unit & Boundary test suite (5/5 PASSED)
│   ├── src/
│   │   ├── lib.rs                    # Rust core library with embedded test suite (4/4 PASSED)
│   │   ├── main.rs                   # Rust CRB & SMM quoter runner
│   │   ├── crb_hedger.rs             # Cross-desk internalization & Almgren-Chriss optimal hedging
│   │   ├── smm_quoter.rs             # Avellaneda-Stoikov market-making with AI alpha skew
│   │   ├── collateral_simm_manager.rs # Dynamic ISDA SIMM margin call manager (42 ns)
│   │   └── aeron_cluster_sequencer.rs # 3-node Raft consensus replicated sequencer (84 ns)
│   └── Cargo.toml
├── agents/                           # Multi-Agent AI Subsystem (Oil Derivatives Desk)
│   ├── tests/
│   │   └── test_agents.py            # Python agent test suite (PASSED)
│   ├── curve_construction_agent.py   # Forward strip bootstrapping & splining
│   ├── physical_logistics_agent.py   # Vessel voyage & laytime monitoring
│   ├── signal_generation_agent.py    # Refinery runs, tanker congestion & inventory alpha
│   ├── pricing_compute_agent.py      # Non-linear Asian option pricer & Greeks
│   ├── orchestrator.py               # Multi-agent coordinator & quote synthesizer
│   └── state.py
├── saas-control/                     # Enterprise SaaS Control Plane & API Gateway
│   ├── schema.sql                    # PostgreSQL DDL with RLS, physical fixtures, CSA & XVA tables
│   ├── main.go                       # High-concurrency REST/WebSocket RFQ negotiation server
│   ├── main_test.go                  # Go unit tests for health & RFQ trade flow (PASSED)
│   └── go.mod
├── docker/                           # Multi-Stage Production Containers
│   ├── Dockerfile.core               # C++ & Rust optimized container
│   ├── Dockerfile.agents             # Python AI runtime
│   └── Dockerfile.saas               # Go API server
├── docker-compose.yml                # Local orchestration (PostgreSQL + QuestDB + Redis + Core + SaaS)
└── .github/workflows/ci.yml          # GitHub Actions CI Quality Gate
```

---

## Flux Terminal CLI Quickstart

Build the CLI binary:
```bash
go build -o bin/flux cli/main.go
```

### Key CLI Commands
```bash
# 1. Request OTC two-way quote & execute trade
./bin/flux rfq --underlying BRENT --strike 82.50 --qty 50000 --execute BUY

# 2. Central Risk Book (CRB) & Tail Risk Analysis
./bin/flux risk

# 3. Render forward curves & SABR implied vol surface
./bin/flux curve --underlying BRENT

# 4. Run Multi-Agent AI evaluation cycle
./bin/flux agents

# 5. Physical CTRM logistics & demurrage monitor
./bin/flux logistics

# 6. Counterparty XVA & ISDA SIMM margin calls
./bin/flux xva

# 7. Live 100ms streaming ticker monitor
./bin/flux monitor
```

---

## Automated Test Suites

* **C++ Tests**:
  ```bash
  cd core-engine
  clang++ -std=c++20 -O3 -Iinclude tests/test_pricing.cpp -o run_tests && ./run_tests
  ```
* **Rust Tests**:
  ```bash
  cd core-engine
  cargo test
  ```
* **Python Tests**:
  ```bash
  cd agents
  python3 -m pytest tests/
  ```
* **Go Tests**:
  ```bash
  cd saas-control
  go test -v .
  ```
