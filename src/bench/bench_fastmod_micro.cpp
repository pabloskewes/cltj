// Micro-benchmark: fastmod_u64 vs native % in the MPHF hash regime.
// Measures (a * x) % p with a < p < 2^32, x < 2^32, so a*x < 2^64.

#include <hashing/mixers.hpp>
#include <hashing/mphf_utils.hpp>

#include <fastmod.h>

#include <CLI11.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

using hrc = std::chrono::high_resolution_clock;

struct Result {
    std::string label;
    long long total_ns;
    size_t iterations;
    double ns_per_op;
};

// Generate (p, a, x[]) combos in the real hash regime.
// n_keys sizes the primes (p ~ 0.42*n), batch_size is how many combos we measure.
// Keys go through premix32, same as the MPHF.
struct Params {
    uint64_t p;
    uint64_t a;
    std::vector<uint64_t> x;
};

std::vector<Params> gen_params(size_t n_keys, size_t batch_size, uint64_t seed) {
    uint64_t target =
        std::max<uint64_t>(3, static_cast<uint64_t>(std::ceil(1.25 * static_cast<double>(n_keys)) + 2) / 3);
    uint64_t base = cltj::hashing::next_prime(target);

    std::mt19937_64 rng(seed);
    std::vector<Params> params;
    params.reserve(batch_size);

    uint64_t p = base;
    for (size_t i = 0; i < batch_size; ++i) {
        Params hp;
        hp.p = p;

        std::uniform_int_distribution<uint64_t> distA(1, p - 1);
        hp.a = distA(rng);

        hp.x.resize(n_keys);
        std::uniform_int_distribution<uint32_t> distKey;
        for (size_t j = 0; j < n_keys; ++j) {
            hp.x[j] = cltj::hashing::premix32(distKey(rng));
        }

        params.push_back(hp);
        p = cltj::hashing::next_prime(p + 1);
    }

    return params;
}

Result bench_native(const std::vector<Params>& params) {
    auto start = hrc::now();

    volatile uint64_t sink = 0;
    size_t ops = 0;

    for (const auto& hp : params) {
        uint64_t a = hp.a;
        uint64_t p_val = hp.p;
        for (uint64_t x : hp.x) {
            uint64_t r = (x * a) % p_val;
            sink ^= r;
            ++ops;
        }
    }

    auto end = hrc::now();
    (void)sink;

    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return {"native %", ns, ops, static_cast<double>(ns) / static_cast<double>(ops)};
}

Result bench_fastmod(const std::vector<Params>& params) {
    std::vector<__uint128_t> M;
    M.reserve(params.size());
    for (const auto& hp : params) {
        M.push_back(fastmod::computeM_u64(hp.p));
    }

    auto start = hrc::now();

    volatile uint64_t sink = 0;
    size_t ops = 0;

    for (size_t i = 0; i < params.size(); ++i) {
        uint64_t a = params[i].a;
        uint64_t p_val = params[i].p;
        __uint128_t m_val = M[i];
        for (uint64_t x : params[i].x) {
            uint64_t prod = x * a;
            uint64_t r = fastmod::fastmod_u64(prod, m_val, p_val);
            sink ^= r;
            ++ops;
        }
    }

    auto end = hrc::now();
    (void)sink;

    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return {"fastmod_u64", ns, ops, static_cast<double>(ns) / static_cast<double>(ops)};
}

bool verify(const std::vector<Params>& params) {
    std::vector<__uint128_t> M;
    M.reserve(params.size());
    for (const auto& hp : params) {
        M.push_back(fastmod::computeM_u64(hp.p));
    }

    size_t checked = 0;
    for (size_t i = 0; i < params.size(); ++i) {
        uint64_t a = params[i].a;
        uint64_t p_val = params[i].p;
        __uint128_t m_val = M[i];
        for (uint64_t x : params[i].x) {
            uint64_t expected = (x * a) % p_val;
            uint64_t got = fastmod::fastmod_u64(x * a, m_val, p_val);
            if (expected != got) {
                std::cerr << "MISMATCH: a=" << a << " x=" << x << " p=" << p_val << " native=" << expected
                          << " fastmod=" << got << std::endl;
                return false;
            }
            ++checked;
        }
    }

    std::cout << "Correctness: " << checked << " ops, all equal." << std::endl;
    return true;
}

struct Config {
    size_t n = 10'000'000;
    size_t batch = 10;
    uint64_t seed = 42;
};

int run(const Config& cfg) {
    std::cout << "=== fastmod_u64 micro-bench ===\n"
              << "n=" << cfg.n << " batch=" << cfg.batch << " seed=" << cfg.seed << std::endl;

    // Sanity: correctness on small batch first.
    {
        auto p = gen_params(1000, 2, cfg.seed);
        if (!verify(p)) {
            std::cerr << "FAIL" << std::endl;
            return 1;
        }
    }

    auto params = gen_params(cfg.n, cfg.batch, cfg.seed);
    std::cout << "primes [" << params.front().p << ", " << params.back().p << "]" << std::endl;

    auto native = bench_native(params);
    std::cout << native.label << ": " << native.total_ns << " ns / " << native.iterations << " = "
              << native.ns_per_op << " ns/op" << std::endl;

    auto fm = bench_fastmod(params);
    std::cout << fm.label << ": " << fm.total_ns << " ns / " << fm.iterations << " = " << fm.ns_per_op
              << " ns/op" << std::endl;

    double ratio = fm.ns_per_op / native.ns_per_op;
    if (ratio < 1.0) {
        std::cout << "fastmod wins: " << (1.0 / ratio) << "x faster" << std::endl;
    } else {
        std::cout << "native wins: " << ratio << "x faster" << std::endl;
    }

    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;

    CLI::App app{"fastmod_u64 vs native % micro-bench"};
    app.add_option("-n", cfg.n, "Logical N (determines prime sizes)")->capture_default_str();
    app.add_option("--batch", cfg.batch, "Number of (p,a,x[]) combos")->capture_default_str();
    app.add_option("--seed", cfg.seed, "RNG seed")->capture_default_str();

    CLI11_PARSE(app, argc, argv);
    return run(cfg);
}
