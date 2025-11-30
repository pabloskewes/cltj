// Benchmark MPHF hashing strategies: build time, query time (h(k)), space.
// Results are written to a CSV file via util::CSVWriter.

#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/baseline.hpp>
#include <hashing/storage/packed_trit.hpp>
#include <hashing/storage/glgh.hpp>
#include <hashing/storage/b_strategy.hpp>

#include <CLI11.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

using cltj::hashing::BaselineStorage;
using cltj::hashing::CompressedBitvector;
using cltj::hashing::ExplicitBitvector;
using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::PackedTritStorage;
using cltj::hashing::policies::NoFingerprints;

namespace {

// Generate a vector of unique random keys in a reasonable range.
static std::vector<uint64_t> generate_unique_keys(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::unordered_set<uint64_t> unique_keys;
    unique_keys.reserve(n * 2);
    // Generate keys in a range up to 100*n to reduce collisions during generation.
    std::uniform_int_distribution<uint64_t> dist(1, n * 100);
    while (unique_keys.size() < n) {
        unique_keys.insert(dist(rng));
    }
    return std::vector<uint64_t>(unique_keys.begin(), unique_keys.end());
}

struct BenchConfig {
    std::string csv_path = "data/mphf_hashing_bench.csv";
    // Default sizes up to 50M keys
    std::vector<size_t> ns = {
        100, 1000, 10000, 100000, 1000000, 2000000, 5000000, 10000000, 20000000, 50000000
    };
    uint64_t base_seed = 42;
    size_t query_reps = 5;  // how many times to repeat the full query loop
    // Which strategies to run; by default run all internal ones
    std::vector<std::string> strategies = {
        "BaselineStorage", "PackedTritStorage_ExplicitB", "PackedTritStorage_CompressedB", "GlGhStorage"
    };
};

struct BenchResult {
    std::string strategy;
    size_t n = 0;
    uint64_t seed = 0;
    long long build_time_us = 0;
    long long query_time_us = 0;
    size_t size_bytes = 0;
    double bits_per_key = 0.0;
    uint32_t m = 0;
    double overhead_m_over_n = 0.0;
    int retries = 0;
    bool build_success = false;
};

template <typename StorageStrategy>
BenchResult run_bench_case(const std::string& strategy_name, size_t n, uint64_t seed, size_t query_reps) {
    BenchResult result;
    result.strategy = strategy_name;
    result.n = n;
    result.seed = seed;

    auto keys = generate_unique_keys(n, seed);

    MPHF<StorageStrategy, NoFingerprints> mphf;

    // 1. Measure build time
    auto start_build = std::chrono::high_resolution_clock::now();
    result.build_success = mphf.build(keys);
    auto end_build = std::chrono::high_resolution_clock::now();
    result.build_time_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end_build - start_build).count();

    if (!result.build_success) {
        result.retries = mphf.retry_count();
        return result;
    }

    result.retries = mphf.retry_count();

    // 2. Measure query time: h(k) over all keys, repeated query_reps times.
    volatile uint64_t sink = 0;  // prevent the compiler from optimizing queries away
    auto start_query = std::chrono::high_resolution_clock::now();
    for (size_t rep = 0; rep < query_reps; ++rep) {
        for (uint64_t k : keys) {
            uint32_t h = mphf.query(k);
            sink ^= h;
        }
    }
    auto end_query = std::chrono::high_resolution_clock::now();
    (void)sink;
    result.query_time_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end_query - start_query).count();

    // 3. Space usage (NoFingerprints, so this is pure MPHF core space).
    auto breakdown = mphf.get_size_breakdown();
    result.size_bytes = breakdown.total_bytes();
    result.bits_per_key = (result.size_bytes * 8.0) / static_cast<double>(n);
    result.m = mphf.m();
    result.overhead_m_over_n = static_cast<double>(result.m) / static_cast<double>(n);

    return result;
}

int run_bench(const BenchConfig& cfg) {
    std::ofstream out(cfg.csv_path);
    out << "strategy,n,seed,build_time_us,query_time_us,query_time_ns_per_key,"
           "size_bytes,bits_per_key,m,m_over_n,retries,build_success\n";

    for (size_t n : cfg.ns) {
        std::cout << "=== Benchmarking n = " << n << " ===" << std::endl;

        // For reproducibility, vary seed per n but in a deterministic way.
        uint64_t seed = cfg.base_seed + n;

        auto write_row = [&](const BenchResult& r) {
            double query_time_ns_per_key = (static_cast<double>(r.query_time_us) * 1000.0) /
                static_cast<double>(r.n * std::max<size_t>(1, cfg.query_reps));
            out << r.strategy << "," << r.n << "," << r.seed << "," << r.build_time_us << ","
                << r.query_time_us << "," << query_time_ns_per_key << "," << r.size_bytes << ","
                << r.bits_per_key << "," << r.m << "," << r.overhead_m_over_n << "," << r.retries << ","
                << (r.build_success ? 1 : 0) << "\n";
        };

        auto should_run = [&](const std::string& name) {
            return std::find(cfg.strategies.begin(), cfg.strategies.end(), name) != cfg.strategies.end();
        };

        // 4 storage combinations (no fingerprints):
        // 1) BaselineStorage
        if (should_run("BaselineStorage")) {
            auto r_baseline = run_bench_case<BaselineStorage>("BaselineStorage", n, seed, cfg.query_reps);
            write_row(r_baseline);
        }

        // 2) PackedTritStorage with explicit B (ExplicitBitvector)
        if (should_run("PackedTritStorage_ExplicitB")) {
            auto r_packed_explicit = run_bench_case<PackedTritStorage<ExplicitBitvector>>(
                "PackedTritStorage_ExplicitB", n, seed, cfg.query_reps
            );
            write_row(r_packed_explicit);
        }

        // 3) PackedTritStorage with compressed B (CompressedBitvector)
        if (should_run("PackedTritStorage_CompressedB")) {
            auto r_packed_compressed = run_bench_case<PackedTritStorage<CompressedBitvector>>(
                "PackedTritStorage_CompressedB", n, seed, cfg.query_reps
            );
            write_row(r_packed_compressed);
        }

        // 4) GlGhStorage (Gl/Gh on-the-fly B)
        if (should_run("GlGhStorage")) {
            auto r_glgh = run_bench_case<GlGhStorage>("GlGhStorage", n, seed, cfg.query_reps);
            write_row(r_glgh);
        }
    }

    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    BenchConfig cfg;

    CLI::App app{"Benchmark MPHF hashing strategies (build, query, space) with CSV output"};
    app.add_option("--output", cfg.csv_path, "Output CSV path")->capture_default_str();
    app.add_option("--sizes", cfg.ns, "List of n values to benchmark (space-separated)")
        ->capture_default_str();
    app.add_option("--query-reps", cfg.query_reps, "Number of times to repeat the full query loop")
        ->capture_default_str();
    app.add_option("--seed", cfg.base_seed, "Base seed for key generation")->capture_default_str();
    app.add_option(
           "--strategies",
           cfg.strategies,
           "Strategies to run (any of: BaselineStorage, PackedTritStorage_ExplicitB, "
           "PackedTritStorage_CompressedB, GlGhStorage)"
    )
        ->capture_default_str();

    CLI11_PARSE(app, argc, argv);

    return run_bench(cfg);
}
