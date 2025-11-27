#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/packed_trit.hpp>
#include <iomanip>
#include <sstream>
#include <vector>
#include <random>
#include <iostream>

using cltj::hashing::BaselineStorage;
using cltj::hashing::MPHF;
using cltj::hashing::PackedTritStorage;
using cltj::hashing::policies::NoFingerprints;

int main() {
    std::cout << "=== MPHF Size Breakdown Analysis ===" << std::endl;

    // Test with different sizes
    std::vector<size_t> test_sizes = {1000, 10000, 100000};

    for (size_t n : test_sizes) {
        std::cout << "--- Analysis for n = " << n << " ---" << std::endl;

        // Generate test keys
        std::mt19937_64 rng(42);
        std::vector<uint64_t> keys;
        for (size_t i = 0; i < n; ++i) {
            keys.push_back(rng());
        }

        // Test BaselineStorage
        {
            std::cout << "\n[BaselineStorage]" << std::endl;
            MPHF<BaselineStorage, NoFingerprints> mphf;
            bool success = mphf.build(keys);

            if (!success) {
                std::cout << "Build failed!" << std::endl;
                continue;
            }

            auto breakdown = mphf.get_size_breakdown();
            double bits_per_key = (breakdown.total_bytes() * 8.0) / n;

            std::cout << "Size breakdown:" << std::endl;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            std::cout << "  G array (2-bit values): " << breakdown.g_bytes << " bytes ("
                      << (breakdown.g_bytes * 8.0 / n) << " bits/key)" << std::endl;
            std::cout << "  Used positions bitvector: " << breakdown.used_pos_bytes << " bytes ("
                      << (breakdown.used_pos_bytes * 8.0 / n) << " bits/key)" << std::endl;
            std::cout << "  Rank support: " << breakdown.rank_bytes << " bytes ("
                      << (breakdown.rank_bytes * 8.0 / n) << " bits/key)" << std::endl;
            oss << bits_per_key;
            std::cout << "  TOTAL: " << breakdown.total_bytes() << " bytes (" << oss.str() << " bits/key)"
                      << std::endl;
        }

        // Test PackedTritStorage
        {
            std::cout << "\n[PackedTritStorage]" << std::endl;
            MPHF<PackedTritStorage, NoFingerprints> mphf;
            bool success = mphf.build(keys);

            if (!success) {
                std::cout << "Build failed!" << std::endl;
                continue;
            }

            auto breakdown = mphf.get_size_breakdown();
            double bits_per_key = (breakdown.total_bytes() * 8.0) / n;

            std::cout << "Size breakdown:" << std::endl;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            std::cout << "  G' array (packed trits): " << breakdown.g_bytes << " bytes ("
                      << (breakdown.g_bytes * 8.0 / n) << " bits/key)" << std::endl;
            std::cout << "  Used positions bitvector: " << breakdown.used_pos_bytes << " bytes ("
                      << (breakdown.used_pos_bytes * 8.0 / n) << " bits/key)" << std::endl;
            std::cout << "  Rank support: " << breakdown.rank_bytes << " bytes ("
                      << (breakdown.rank_bytes * 8.0 / n) << " bits/key)" << std::endl;
            oss << bits_per_key;
            std::cout << "  TOTAL: " << breakdown.total_bytes() << " bytes (" << oss.str() << " bits/key)"
                      << std::endl;
        }

        std::cout << std::endl;
    }

    return 0;
}
