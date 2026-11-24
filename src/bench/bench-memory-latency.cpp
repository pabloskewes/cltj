// Memory latency of the machine, to read the per-node lookup costs against:
// a pointer chase over one random cycle of cache lines (each load depends on
// the previous one), and random independent loads with and without a
// serializing fence between them, over arrays of growing size.

#include <CLI11.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
static inline void serialize_fence() { _mm_lfence(); }
#else
static inline void serialize_fence() { asm volatile("dsb ish\n\tisb" ::: "memory"); }
#endif

namespace {

struct alignas(64) Line {
    uint64_t next;
    uint64_t pad[7];
};

template <bool Fence>
uint64_t random_loads(const std::vector<Line>& a, const std::vector<uint32_t>& idx) {
    uint64_t acc = 0;
    for (uint32_t i : idx) {
        acc += a[i].next;
        if (Fence)
            serialize_fence();
    }
    return acc;
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Pointer chase and random loads over arrays of growing size"};
    std::string out_path, sizes = "16384,262144,4194304,16777216,67108864,268435456,1073741824";
    uint64_t loads = 1 << 24;
    uint32_t reps = 5;
    app.add_option("-o,--out", out_path, "CSV")->required();
    app.add_option("--sizes", sizes, "Array sizes in bytes, comma separated");
    app.add_option("--loads", loads, "Loads per measurement");
    app.add_option("--reps", reps, "Repetitions");
    CLI11_PARSE(app, argc, argv);

    std::vector<uint64_t> sz;
    std::stringstream ss(sizes);
    std::string tok;
    while (std::getline(ss, tok, ','))
        sz.push_back(std::stoull(tok));

    std::ofstream out(out_path, std::ios::app);
    std::mt19937_64 rng(7);
    for (uint64_t bytes : sz) {
        const uint64_t n = std::max<uint64_t>(2, bytes / sizeof(Line));
        std::vector<Line> a(n);
        std::vector<uint64_t> perm(n);
        std::iota(perm.begin(), perm.end(), 0);
        for (uint64_t i = n - 1; i > 0; --i)  // Sattolo: one cycle over all lines
            std::swap(perm[i], perm[rng() % i]);
        for (uint64_t i = 0; i < n; ++i)
            a[i].next = perm[i];
        std::vector<uint32_t> idx(loads);
        for (auto& i : idx)
            i = rng() % n;
        for (uint32_t rep = 0; rep < reps; ++rep) {
            uint64_t p = 0;
            for (uint64_t k = 0; k < std::min<uint64_t>(n, loads); ++k)
                p = a[p].next;
            auto t0 = std::chrono::steady_clock::now();
            for (uint64_t k = 0; k < loads; ++k)
                p = a[p].next;
            auto t1 = std::chrono::steady_clock::now();
            uint64_t s1 = random_loads<false>(a, idx);
            auto t2 = std::chrono::steady_clock::now();
            uint64_t s2 = random_loads<true>(a, idx);
            auto t3 = std::chrono::steady_clock::now();
            auto ns = [&](auto x, auto y) { return std::chrono::duration<double, std::nano>(y - x).count() / loads; };
            out << bytes << ',' << rep << ",chase," << ns(t0, t1) << ',' << p << '\n';
            out << bytes << ',' << rep << ",random_indep," << ns(t1, t2) << ',' << s1 << '\n';
            out << bytes << ',' << rep << ",random_fence," << ns(t2, t3) << ',' << s2 << '\n';
            out.flush();
            std::cerr << bytes << " rep " << rep << ": chase " << ns(t0, t1) << " ns, indep " << ns(t1, t2)
                      << " ns, fence " << ns(t2, t3) << " ns" << std::endl;
        }
    }
    return 0;
}
