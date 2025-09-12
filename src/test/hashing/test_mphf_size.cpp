// Standalone test for MPHF size analysis
#include <include/hashing/mphf_bdz.hpp>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

using cltj::hashing::MPHF;

static std::vector<uint64_t> generate_reasonable_keys(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::unordered_set<uint64_t> unique_keys;
    std::uniform_int_distribution<uint64_t> dist(1, 1000000);
    while (unique_keys.size() < n) {
        unique_keys.insert(dist(rng));
    }
    return std::vector<uint64_t>(unique_keys.begin(), unique_keys.end());
}

int main() {
    std::cout << "========== MPHF SIZE ANALYSIS ==========\n\n";
    
    std::vector<size_t> test_sizes = {100, 1000, 10000, 100000, 1000000};
    
    for (size_t n : test_sizes) {
        if (n >= 100000) {
            std::cout << "Skipping n=" << n << " due to large size.\n";
            continue;
        }
        auto keys = generate_reasonable_keys(n, 42 + n);
        MPHF mphf;
        bool ok = mphf.build(keys);

        if (!ok) {
            std::cout << "n=" << n << ": Build FAILED\n\n";
            continue;
        }

        // Measure size using SDSL
        size_t size_bytes = sdsl::size_in_bytes(mphf);
        double bits_per_key = (size_bytes * 8.0) / n;
        double overhead = (double)mphf.m() / n;

        std::cout << "n=" << n << ":\n";
        std::cout << "  Size: " << size_bytes << " bytes (" << size_bytes / 1024.0 << " KB)\n";
        std::cout << "  Bits per key: " << std::fixed << std::setprecision(2) << bits_per_key << "\n";
        std::cout << "  m/n overhead: " << std::fixed << std::setprecision(3) << overhead << "\n";
        std::cout << "  Retries: " << mphf.retry_count() << "\n\n";
    }

    return 0;
}
