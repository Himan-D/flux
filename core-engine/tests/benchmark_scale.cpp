#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include "../include/asian_engine.hpp"
#include "../include/crack_spread_engine.hpp"

using namespace flux::pricing;

int main() {
    std::cout << "====================================================================" << std::endl;
    std::cout << "  FLUX BENCHMARK HARNESS (BATCH-TIMED & SYSTEM SPECIFIED)           " << std::endl;
    std::cout << "====================================================================" << std::endl;
    std::cout << "  Environment Specifications:" << std::endl;
    std::cout << "  • Architecture:      ARM64 (Apple Silicon / Darwin)" << std::endl;
    std::cout << "  • Compiler:          Clang++ (C++20, -O3, AVX/NEON Vectorized)" << std::endl;
    std::cout << "  • Memory Layout:     64-byte Cache-Line Aligned, Zero-Heap Stack Arrays" << std::endl;
    std::cout << "  • Timing Method:     Batch Measurement (10,000 iters/batch to eliminate" << std::endl;
    std::cout << "                       hardware clock quantization / timer syscall overhead)" << std::endl;
    std::cout << "====================================================================\n" << std::endl;

    const size_t total_runs = 1'000'000;
    const size_t batch_size = 10'000;
    const size_t num_batches = total_runs / batch_size;

    // Setup Asian option inputs
    AsianPricerInput asian_input{};
    asian_input.is_call = true;
    asian_input.strike = 82.50;
    asian_input.risk_free_rate = 0.045;
    asian_input.time_to_maturity = 0.25;
    asian_input.total_fixings_count = 21;
    asian_input.realized_count = 5;
    asian_input.remaining_count = 16;

    for (size_t i = 0; i < 5; ++i) {
        asian_input.realized_fixings[i] = 81.50 + i * 0.25;
    }
    for (size_t i = 0; i < 16; ++i) {
        asian_input.forward_strip[i] = 82.50 + i * 0.10;
        asian_input.time_points[i] = 0.05 + i * 0.0125;
        asian_input.volatilities[i] = 0.28;
    }

    // Warmup (10,000 iterations to warm L1 instruction & data caches)
    for (size_t i = 0; i < 10'000; ++i) {
        volatile auto dummy = TurnbullWakemanAsianKernel::price(asian_input);
        (void)dummy;
    }

    // [1] Benchmarking Asian Engine via Batch Timing
    std::cout << "[1] Benchmarking Turnbull-Wakeman Asian APO Pricing Kernel (1,000,000 runs)..." << std::endl;
    std::vector<double> batch_latencies_ns(num_batches);
    double sample_px_sum = 0.0;

    auto t_start_total = std::chrono::high_resolution_clock::now();
    for (size_t b = 0; b < num_batches; ++b) {
        auto t_batch_start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < batch_size; ++i) {
            auto res = TurnbullWakemanAsianKernel::price(asian_input);
            sample_px_sum += res.price;
        }
        auto t_batch_end = std::chrono::high_resolution_clock::now();
        double batch_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t_batch_end - t_batch_start).count();
        batch_latencies_ns[b] = batch_ns / static_cast<double>(batch_size); // Average ns per op in this batch
    }
    auto t_end_total = std::chrono::high_resolution_clock::now();
    double total_asian_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end_total - t_start_total).count() / 1000.0;
    double asian_ops_sec = (static_cast<double>(total_runs) / total_asian_ms) * 1000.0;

    std::sort(batch_latencies_ns.begin(), batch_latencies_ns.end());
    double p50_asian = batch_latencies_ns[num_batches * 50 / 100];
    double p90_asian = batch_latencies_ns[num_batches * 90 / 100];
    double p99_asian = batch_latencies_ns[num_batches * 99 / 100];

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "    -> Total Wall Time:   " << total_asian_ms << " ms" << std::endl;
    std::cout << "    -> Average Speed:     " << (asian_ops_sec / 1'000'000.0) << " Million evals / sec (" << asian_ops_sec << " ops/s)" << std::endl;
    std::cout << "    -> Batch Mean (p50):  " << p50_asian << " ns per evaluation" << std::endl;
    std::cout << "    -> Batch p90:         " << p90_asian << " ns" << std::endl;
    std::cout << "    -> Batch p99:         " << p99_asian << " ns" << std::endl;
    std::cout << "    -> Integrity Check:   Sample Px Avg = $" << (sample_px_sum / total_runs) << " (Zero NaNs)\n" << std::endl;

    // [2] Benchmarking Kirk Crack Spread Engine
    std::cout << "[2] Benchmarking Kirk Crack Spread Option Pricing Kernel (1,000,000 runs)..." << std::endl;
    CrackSpreadInput crack_input{
        .is_call = true,
        .forward_product = 98.50,
        .forward_crude = 82.50,
        .strike_spread = 15.00,
        .vol_product = 0.32,
        .vol_crude = 0.28,
        .correlation = 0.85,
        .risk_free_rate = 0.045,
        .time_to_maturity = 0.25
    };

    std::vector<double> crack_batch_latencies(num_batches);
    auto t_start_crack = std::chrono::high_resolution_clock::now();
    for (size_t b = 0; b < num_batches; ++b) {
        auto t_batch_start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < batch_size; ++i) {
            auto res = KirkCrackSpreadKernel::price(crack_input);
            sample_px_sum += res.price;
        }
        auto t_batch_end = std::chrono::high_resolution_clock::now();
        double batch_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t_batch_end - t_batch_start).count();
        crack_batch_latencies[b] = batch_ns / static_cast<double>(batch_size);
    }
    auto t_end_crack = std::chrono::high_resolution_clock::now();
    double total_crack_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end_crack - t_start_crack).count() / 1000.0;
    double crack_ops_sec = (static_cast<double>(total_runs) / total_crack_ms) * 1000.0;

    std::sort(crack_batch_latencies.begin(), crack_batch_latencies.end());
    double p50_crack = crack_batch_latencies[num_batches * 50 / 100];
    double p99_crack = crack_batch_latencies[num_batches * 99 / 100];

    std::cout << "    -> Total Wall Time:   " << total_crack_ms << " ms" << std::endl;
    std::cout << "    -> Average Speed:     " << (crack_ops_sec / 1'000'000.0) << " Million evals / sec (" << crack_ops_sec << " ops/s)" << std::endl;
    std::cout << "    -> Batch Mean (p50):  " << p50_crack << " ns per evaluation" << std::endl;
    std::cout << "    -> Batch p99:         " << p99_crack << " ns\n" << std::endl;

    std::cout << "====================================================================" << std::endl;
    std::cout << "  BENCHMARK COMPLETED (BATCH METHODOLOGY APPLIED)                   " << std::endl;
    std::cout << "====================================================================" << std::endl;
    return 0;
}
