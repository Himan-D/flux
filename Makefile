# ==============================================================================
# FLUX QUANTITATIVE TRADING PLATFORM — UNIFIED MAKEFILE
# ==============================================================================

.PHONY: all build test benchmark demo docker-up clean

all: build test

build:
	@echo "==> Building C++20 Pricing Kernels & Shared Library..."
	@mkdir -p core-engine/lib
	@cd core-engine && clang++ -std=c++20 -O3 -shared -fPIC -Iinclude src/c_api.cpp -o lib/libflux_pricing.dylib || clang++ -std=c++20 -O3 -shared -fPIC -Iinclude src/c_api.cpp -o lib/libflux_pricing.so
	@cd core-engine && clang++ -std=c++20 -O3 -Wall -Wextra -Werror -Iinclude tests/test_pricing.cpp -o run_tests
	@cd core-engine && clang++ -std=c++20 -O3 -Iinclude tests/test_reference_validation.cpp -o tests/test_reference_validation
	@cd core-engine && clang++ -std=c++20 -O3 -Iinclude tests/test_quant_depth.cpp -o tests/test_quant_depth
	@cd core-engine && clang++ -std=c++20 -O3 -Iinclude tests/benchmark_scale.cpp -o tests/benchmark_scale
	@echo "==> Building Rust Core Engine & Aeron Sequencer..."
	@cd core-engine && cargo build --release
	@echo "==> Building Go Terminal CLI & SaaS Gateway..."
	@mkdir -p bin
	@go build -o bin/flux cli/*.go
	@go build -o bin/flux-server saas-control/*.go
	@echo "==> Build Complete: ./bin/flux"

test:
	@echo "==> [1/6] Running C++20 Unit Tests..."
	@./core-engine/run_tests
	@echo "==> [2/6] Running C++20 Reference Validation (Monte Carlo Tolerance)..."
	@./core-engine/tests/test_reference_validation
	@echo "==> [3/6] Running C++20 Advanced Quant Depth (SABR, 3:2:1 Crack, Cholesky MC)..."
	@./core-engine/tests/test_quant_depth
	@echo "==> [4/6] Running Rust Core & FIX Order State Machine Tests..."
	@cd core-engine && cargo test
	@echo "==> [5/6] Running Python Multi-Agent Pytest Suite..."
	@python3 agents/tests/test_agents.py
	@echo "==> [6/6] Running Go SaaS Gateway Tests..."
	@cd saas-control && go test -v .

benchmark: build
	@./bin/flux benchmark

demo: build
	@./scripts/demo.sh

docker-up:
	@docker-compose up -d --build

clean:
	@rm -rf bin/ core-engine/run_tests core-engine/tests/benchmark_scale core-engine/target/
