// Micro-benchmark: fastmod_u64 vs native % in the MPHF hash regime.
// Measures (a * x) % p with a < p < 2^32, x < 2^32, so a*x < 2^64.
//
// Design (why the bench is shaped this way):
//  - p is runtime-variable per combo, so native % compiles to a real hardware divide.
//    A compile-time-constant divisor would let the compiler emit its own magic-multiply
//    (i.e. fastmod), making the comparison meaningless.
//  - The key buffer is small and cache-resident: this bench isolates COMPUTE cost only.
//    Out-of-cache / memory-bandwidth effects belong to the end-to-end --glgh benchmark,
//    not here. `n` sizes the prime (matching the real regime), it does NOT size the buffer.
//  - Anti-DCE via a single volatile store AFTER each timed loop, not one per iteration
//    (a per-iteration volatile RMW adds fixed overhead that compresses the ratio).
//  - Each measurement is warmed up, repeated `trials` times, and reported as the median;
//    single-pass timings on this workload swing wildly (turbo ramp, scheduling).

#include <hashing/mixers.hpp>
#include <hashing/mphf_utils.hpp>

#include <fastmod.h>

#include <CLI11.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <random>
#include <vector>

namespace {

using hrc = std::chrono::high_resolution_clock;

// One (p, a, key-buffer) combo. M is the fastmod reciprocal, precomputed once.
struct Combo {
    uint64_t p;
    uint64_t a;
    __uint128_t M;
    std::vector<uint64_t> x;  // cache-resident buffer of premixed keys
};

// Prime magnitude for a logical N, mirroring MPHF::compute_target_segment:
// m ~ 1.25*n slots split across 3 segments, so each prime ~ 0.42*n.
uint64_t prime_base_for_n(size_t n) {
    uint64_t target_m = static_cast<uint64_t>(std::ceil(1.25 * static_cast<double>(n)));
    uint64_t target = std::max<uint64_t>(3, (target_m + 2) / 3);
    return cltj::hashing::next_prime(target);
}

std::vector<Combo> gen_combos(size_t n_for_prime, size_t keys, size_t batch, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<Combo> combos;
    combos.reserve(batch);

    uint64_t p = prime_base_for_n(n_for_prime);
    for (size_t i = 0; i < batch; ++i) {
        Combo c;
        c.p = p;
        c.M = fastmod::computeM_u64(p);

        std::uniform_int_distribution<uint64_t> distA(1, p - 1);
        c.a = distA(rng);

        c.x.resize(keys);
        std::uniform_int_distribution<uint32_t> distKey;
        for (size_t j = 0; j < keys; ++j) {
            c.x[j] = cltj::hashing::premix32(distKey(rng));
        }

        combos.push_back(std::move(c));
        p = cltj::hashing::next_prime(p + 1);
    }
    return combos;
}

// How many passes over the buffers to reach ~target_ops operations.
size_t reps_for(size_t target_ops, size_t batch, size_t keys) {
    size_t per_pass = batch * keys;
    if (per_pass == 0)
        return 1;
    return std::max<size_t>((target_ops + per_pass - 1) / per_pass, 1);
}

// Per-rep salt (xor with the pass index) keeps xv < 2^32 while making each pass distinct,
// so the compiler cannot hoist/CSE the inner work across the reps loop.
double bench_native(const std::vector<Combo>& combos, size_t reps) {
    uint64_t acc = 0;
    auto start = hrc::now();
    for (size_t r = 0; r < reps; ++r) {
        const uint64_t salt = static_cast<uint32_t>(r);
        for (const auto& c : combos) {
            const uint64_t a = c.a;
            const uint64_t p = c.p;
            for (uint64_t x : c.x) {
                const uint64_t xv = x ^ salt;
                acc ^= (xv * a) % p;
            }
        }
    }
    auto end = hrc::now();
    volatile uint64_t sink = acc;  // single anti-DCE store
    (void)sink;

    double ns =
        static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    double ops = static_cast<double>(reps) * static_cast<double>(combos.size()) *
        static_cast<double>(combos.front().x.size());
    return ns / ops;
}

double bench_fastmod(const std::vector<Combo>& combos, size_t reps) {
    uint64_t acc = 0;
    auto start = hrc::now();
    for (size_t r = 0; r < reps; ++r) {
        const uint64_t salt = static_cast<uint32_t>(r);
        for (const auto& c : combos) {
            const uint64_t a = c.a;
            const uint64_t p = c.p;
            const __uint128_t M = c.M;
            for (uint64_t x : c.x) {
                const uint64_t xv = x ^ salt;
                acc ^= fastmod::fastmod_u64(xv * a, M, p);
            }
        }
    }
    auto end = hrc::now();
    volatile uint64_t sink = acc;
    (void)sink;

    double ns =
        static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    double ops = static_cast<double>(reps) * static_cast<double>(combos.size()) *
        static_cast<double>(combos.front().x.size());
    return ns / ops;
}

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    if (n == 0)
        return 0.0;
    return (n % 2) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// Confirm fastmod == native % over the buffer plus boundary x values.
bool verify(const std::vector<Combo>& combos) {
    size_t checked = 0;
    for (const auto& c : combos) {
        std::vector<uint64_t> probes = {0, 1, 2, c.p - 1, c.p + 1, 0xFFFFFFFFull};
        probes.insert(probes.end(), c.x.begin(), c.x.end());
        for (uint64_t xraw : probes) {
            const uint64_t x = static_cast<uint32_t>(xraw);  // mixed keys live in the 32-bit domain
            const uint64_t prod = x * c.a;
            const uint64_t expected = prod % c.p;
            const uint64_t got = fastmod::fastmod_u64(prod, c.M, c.p);
            if (expected != got) {
                std::cerr << "MISMATCH: a=" << c.a << " x=" << x << " p=" << c.p << " native=" << expected
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
    size_t n = 10'000'000;  // sizes the prime (matches the real regime), NOT the buffer
    size_t keys = 4096;  // per-combo key buffer; keys*batch stays cache-resident
    size_t batch = 16;  // number of distinct primes rotated through
    size_t ops = 200'000'000;  // approximate total operations timed per trial
    size_t trials = 7;  // repetitions; the median is reported
    uint64_t seed = 42;
};

int run(const Config& cfg) {
    std::cout << "=== fastmod_u64 micro-bench ===\n"
              << "n(prime)=" << cfg.n << " keys=" << cfg.keys << " batch=" << cfg.batch << " ops~" << cfg.ops
              << " trials=" << cfg.trials << " seed=" << cfg.seed << std::endl;

    auto combos = gen_combos(cfg.n, cfg.keys, cfg.batch, cfg.seed);
    const size_t ws_kb = (cfg.keys * cfg.batch * sizeof(uint64_t)) / 1024;
    std::cout << "primes [" << combos.front().p << ", " << combos.back().p << "]  working set " << ws_kb
              << " KB total" << std::endl;

    if (!verify(combos)) {
        std::cerr << "FAIL" << std::endl;
        return 1;
    }

    const size_t reps = reps_for(cfg.ops, cfg.batch, cfg.keys);

    // Warm-up (untimed): spin up frequency, prime caches.
    bench_native(combos, 1);
    bench_fastmod(combos, 1);

    std::vector<double> nat, fm;
    nat.reserve(cfg.trials);
    fm.reserve(cfg.trials);
    for (size_t t = 0; t < cfg.trials; ++t) {
        nat.push_back(bench_native(combos, reps));  // alternate the two so drift hits both equally
        fm.push_back(bench_fastmod(combos, reps));
    }

    const double nat_med = median(nat);
    const double fm_med = median(fm);
    std::cout << "native %    median: " << nat_med << " ns/op  (min "
              << *std::min_element(nat.begin(), nat.end()) << ", max "
              << *std::max_element(nat.begin(), nat.end()) << ")" << std::endl;
    std::cout << "fastmod_u64 median: " << fm_med << " ns/op  (min "
              << *std::min_element(fm.begin(), fm.end()) << ", max "
              << *std::max_element(fm.begin(), fm.end()) << ")" << std::endl;

    const double ratio = fm_med / nat_med;
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

    CLI::App app{"fastmod_u64 vs native % micro-bench (compute-isolated)"};
    app.add_option("-n", cfg.n, "Logical N (determines prime magnitude)")->capture_default_str();
    app.add_option("--keys", cfg.keys, "Keys per combo (buffer stays cache-resident)")->capture_default_str();
    app.add_option("--batch", cfg.batch, "Number of distinct primes")->capture_default_str();
    app.add_option("--ops", cfg.ops, "Approx total ops timed per trial")->capture_default_str();
    app.add_option("--trials", cfg.trials, "Repetitions (median reported)")->capture_default_str();
    app.add_option("--seed", cfg.seed, "RNG seed")->capture_default_str();

    CLI11_PARSE(app, argc, argv);
    return run(cfg);
}
