#include <hashing/mphf_bdz.hpp>
#include <util/logger.hpp>
#include <iomanip>
#include <sstream>
#include <vector>
#include <random>
#include <iostream>

using cltj::hashing::MPHF;

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

        MPHF mphf;
        bool success = mphf.build(keys);

        if (!success) {
            std::cout << "Build failed!" << std::endl;
            continue;
        }

        // Break down the size components
        size_t g_bytes = sdsl::size_in_bytes(mphf.get_g());
        size_t used_pos_bytes = sdsl::size_in_bytes(mphf.get_used_positions());
        size_t rank_bytes = sdsl::size_in_bytes(mphf.get_rank_support());
        size_t q_bytes = sdsl::size_in_bytes(mphf.get_q());
        size_t other_bytes = sizeof(mphf.m()) + sizeof(mphf.n()) + sizeof(mphf.get_primes()) +
            sizeof(mphf.get_multipliers()) + sizeof(mphf.get_biases()) + sizeof(mphf.get_segment_starts());

        size_t total_bytes = g_bytes + used_pos_bytes + rank_bytes + q_bytes + other_bytes;
        double bits_per_key = (total_bytes * 8.0) / n;
        // Core sizes without fingerprints
        size_t core_bytes = g_bytes + used_pos_bytes + rank_bytes + other_bytes;
        double core_bpk = (core_bytes * 8.0) / n;
        size_t core_struct_bytes = g_bytes + used_pos_bytes + rank_bytes;
        double core_struct_bpk = (core_struct_bytes * 8.0) / n;

        std::cout << "Size breakdown:" << std::endl;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        std::cout << "  G array (2-bit values): " << g_bytes << " bytes (" << (g_bytes * 8.0 / n)
                  << " bits/key)" << std::endl;
        std::cout << "  Used positions bitvector: " << used_pos_bytes << " bytes ("
                  << (used_pos_bytes * 8.0 / n) << " bits/key)" << std::endl;
        std::cout << "  Rank support: " << rank_bytes << " bytes (" << (rank_bytes * 8.0 / n) << " bits/key)"
                  << std::endl;
        std::cout << "  Fingerprints (Q array): " << q_bytes << " bytes (" << (q_bytes * 8.0 / n)
                  << " bits/key)" << std::endl;
        std::cout << "  Other metadata: " << other_bytes << " bytes (" << (other_bytes * 8.0 / n)
                  << " bits/key)" << std::endl;
        oss << bits_per_key;
        std::cout << "  TOTAL: " << total_bytes << " bytes (" << oss.str() << " bits/key)" << std::endl;
        // Extra reports without fingerprints
        {
            std::ostringstream o1, o2;
            o1 << std::fixed << std::setprecision(2) << core_struct_bpk;
            o2 << std::fixed << std::setprecision(2) << core_bpk;
            std::cout << "  TOTAL without fingerprints (G + B + rank): " << core_struct_bytes << " bytes ("
                      << o1.str() << " bits/key)" << std::endl;
            std::cout << "  TOTAL without fingerprints (G + B + rank + metadata): " << core_bytes
                      << " bytes (" << o2.str() << " bits/key)" << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Key insights:" << std::endl;
        std::ostringstream oss2, oss3;
        oss2 << std::fixed << std::setprecision(2) << (mphf.m() * 2.0 / n);
        oss3 << std::fixed << std::setprecision(3) << ((double)mphf.m() / n);
        std::cout << "  - G array: " << mphf.m() << " entries × 2 bits = " << (mphf.m() * 2) << " bits ("
                  << oss2.str() << " bits/key)" << std::endl;
        std::cout << "  - Fingerprints: " << n << " entries × 8 bits = " << (n * 8) << " bits (8.00 bits/key)"
                  << std::endl;
        std::cout << "  - m/n ratio: " << oss3.str() << std::endl;
        std::ostringstream oss4;
        oss4 << std::fixed << std::setprecision(2) << (mphf.m() * 2.0 / n + 8.0);
        std::cout << "  - Theoretical minimum: " << oss4.str() << " bits/key (G array + fingerprints)"
                  << std::endl;
        std::cout << std::endl;
    }

    return 0;
}
