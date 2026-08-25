# ==============================================================================
# FLUX QUANTITATIVE TRADING PLATFORM — UNIFIED MAKEFILE
# ==============================================================================

.PHONY: all build test benchmark demo docker-up clean

all: build test

build:
	@echo "==> Building C++20 Pricing Kernels..."
	@cd core-engine && clang++ -std=c++20 -O3 -Wall -Wextra -Werror -Iinclude tests/test_pricing.cpp -o run_tests
	@cd core-engine && clang++ -std=c++20 -O3 -Iinclude tests/benchmark_scale.cpp -o tests/benchmark_scale
	@echo "==> Building Rust Core Engine & Aeron Sequencer..."
	@cd core-engine && cargo build --release
	@echo "==> Building Go Terminal CLI & SaaS Gateway..."
	@mkdir -p bin
	@go build -o bin/flux cli/*.go
	@go build -o bin/flux-server saas-control/*.go
	@echo "==> Build Complete: ./bin/flux"

test:
	@echo "==> [1/4] Running C++20 Unit Tests..."
	@./core-engine/run_tests
	@echo "==> [2/4] Running Rust Core Tests..."
	@cd core-engine && cargo test
	@echo "==> [3/4] Running Python Multi-Agent Pytest Suite..."
	@python3 agents/tests/test_agents.py
	@echo "==> [4/4] Running Go SaaS Gateway Tests..."
	@cd saas-control && go test -v .

benchmark: build
	@./bin/flux benchmark

demo: build
	@./scripts/demo.sh

docker-up:
	@docker-compose up -d --build

clean:
	@rm -rf bin/ core-engine/run_tests core-engine/tests/benchmark_scale core-engine/target/
