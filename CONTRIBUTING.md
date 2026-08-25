# Contributing to Flux

Thank you for your interest in contributing to **Flux**, the open-source high-performance OTC commodity derivatives trading and Central Risk Book platform.

---

## Development & Testing Standards

Flux enforces strict quantitative precision, zero-allocation microsecond execution, and cross-runtime test verification:

### 1. C++20 Numerical Pricing Kernels
* Must be zero-allocation on the hot path (`noexcept`, `[[nodiscard]]`, `std::array`).
* Cache-line alignment (`alignas(64)`) required on all pricer input structs.
* Test with:
  ```bash
  cd core-engine && clang++ -std=c++20 -O3 -Iinclude tests/test_pricing.cpp -o run_tests && ./run_tests
  ```

### 2. Rust Core Engine & Aeron Sequencer
* Strict memory safety, zero-copy serialization, and lock-free concurrency.
* Test with:
  ```bash
  cd core-engine && cargo test
  cargo run --release  # Scale benchmarks
  ```

### 3. Go SaaS Gateway & Terminal CLI
* Use `sync.Pool` for memory buffer recycling in high-throughput handlers.
* Test with:
  ```bash
  cd saas-control && go test -v .
  go build -o bin/flux cli/*.go && ./bin/flux benchmark
  ```

### 4. Python Multi-Agent AI Subsystem
* Type hints (`mypy`) and asynchronous execution (`pytest`).
* Test with:
  ```bash
  cd agents && python3 -m pytest tests/
  ```

---

## Pull Request Workflow

1. Fork the repository and create your branch from `main`:
   ```bash
   git checkout -b feat/my-new-pricer
   ```
2. Commit with conventional commit messages (`feat:`, `fix:`, `perf:`, `docs:`, `test:`).
3. Ensure all 4 test suites pass before submitting a Pull Request.

---

## License
By contributing to Flux, you agree that your contributions will be licensed under the **Apache License 2.0**.
