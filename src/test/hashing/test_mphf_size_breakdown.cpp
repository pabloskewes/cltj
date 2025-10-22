#include <include/hashing/mphf_bdz.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>

using cltj::hashing::MPHF;

int main() {
    std::cout << "=== MPHF Size Breakdown Analysis ===\n\n";

    // Test with different sizes
    std::vector<size_t> test_sizes = {1000, 10000, 100000};

    for (size_t n : test_sizes) {
        std::cout << "--- Analysis for n = " << n << " ---\n";

        // Generate test keys
        std::mt19937_64 rng(42);
        std::vector<uint64_t> keys;
        for (size_t i = 0; i < n; ++i) {
            keys.push_back(rng());
        }

        MPHF mphf;
        bool success = mphf.build(keys);

        if (!success) {
            std::cout << "Build failed!\n\n";
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

        std::cout << "Size breakdown:\n";
        std::cout << "  G array (2-bit values): " << g_bytes << " bytes (" << std::fixed
                  << std::setprecision(2) << (g_bytes * 8.0 / n) << " bits/key)\n";
        std::cout << "  Used positions bitvector: " << used_pos_bytes << " bytes (" << std::fixed
                  << std::setprecision(2) << (used_pos_bytes * 8.0 / n) << " bits/key)\n";
        std::cout << "  Rank support: " << rank_bytes << " bytes (" << std::fixed << std::setprecision(2)
                  << (rank_bytes * 8.0 / n) << " bits/key)\n";
        std::cout << "  Fingerprints (Q array): " << q_bytes << " bytes (" << std::fixed
                  << std::setprecision(2) << (q_bytes * 8.0 / n) << " bits/key)\n";
        std::cout << "  Other metadata: " << other_bytes << " bytes (" << std::fixed << std::setprecision(2)
                  << (other_bytes * 8.0 / n) << " bits/key)\n";
        std::cout << "  TOTAL: " << total_bytes << " bytes (" << std::fixed << std::setprecision(2)
                  << bits_per_key << " bits/key)\n";

        std::cout << "\nKey insights:\n";
        std::cout << "  - G array: " << mphf.m() << " entries × 2 bits = " << (mphf.m() * 2) << " bits ("
                  << std::fixed << std::setprecision(2) << (mphf.m() * 2.0 / n) << " bits/key)\n";
        std::cout << "  - Fingerprints: " << n << " entries × 8 bits = " << (n * 8) << " bits (" << std::fixed
                  << std::setprecision(2) << (8.0) << " bits/key)\n";
        std::cout << "  - m/n ratio: " << std::fixed << std::setprecision(3) << (double)mphf.m() / n << "\n";
        std::cout << "  - Theoretical minimum: " << std::fixed << std::setprecision(2)
                  << (mphf.m() * 2.0 / n + 8.0) << " bits/key (G array + fingerprints)\n";

        std::cout << "\n";
    }

    return 0;
}

