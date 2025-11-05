#include <hashing/mphf_bdz.hpp>
#include <util/logger.hpp>
#include <iomanip>
#include <sstream>
#include <vector>
#include <random>

using cltj::hashing::MPHF;

int main() {
    LOG_INFO("=== MPHF Size Breakdown Analysis ===");

    // Test with different sizes
    std::vector<size_t> test_sizes = {1000, 10000, 100000};

    for (size_t n : test_sizes) {
        LOG_INFO("--- Analysis for n = " << n << " ---");

        // Generate test keys
        std::mt19937_64 rng(42);
        std::vector<uint64_t> keys;
        for (size_t i = 0; i < n; ++i) {
            keys.push_back(rng());
        }

        MPHF mphf;
        bool success = mphf.build(keys);

        if (!success) {
            LOG_WARN("Build failed!");
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

        LOG_INFO("Size breakdown:");
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        LOG_INFO("  G array (2-bit values): " << g_bytes << " bytes (" << (g_bytes * 8.0 / n) << " bits/key)");
        LOG_INFO("  Used positions bitvector: " << used_pos_bytes << " bytes (" << (used_pos_bytes * 8.0 / n) << " bits/key)");
        LOG_INFO("  Rank support: " << rank_bytes << " bytes (" << (rank_bytes * 8.0 / n) << " bits/key)");
        LOG_INFO("  Fingerprints (Q array): " << q_bytes << " bytes (" << (q_bytes * 8.0 / n) << " bits/key)");
        LOG_INFO("  Other metadata: " << other_bytes << " bytes (" << (other_bytes * 8.0 / n) << " bits/key)");
        oss << bits_per_key;
        LOG_INFO("  TOTAL: " << total_bytes << " bytes (" << oss.str() << " bits/key)");

        LOG_INFO("");
        LOG_INFO("Key insights:");
        std::ostringstream oss2, oss3;
        oss2 << std::fixed << std::setprecision(2) << (mphf.m() * 2.0 / n);
        oss3 << std::fixed << std::setprecision(3) << ((double)mphf.m() / n);
        LOG_INFO("  - G array: " << mphf.m() << " entries × 2 bits = " << (mphf.m() * 2) << " bits (" << oss2.str() << " bits/key)");
        LOG_INFO("  - Fingerprints: " << n << " entries × 8 bits = " << (n * 8) << " bits (8.00 bits/key)");
        LOG_INFO("  - m/n ratio: " << oss3.str());
        std::ostringstream oss4;
        oss4 << std::fixed << std::setprecision(2) << (mphf.m() * 2.0 / n + 8.0);
        LOG_INFO("  - Theoretical minimum: " << oss4.str() << " bits/key (G array + fingerprints)");
        LOG_INFO("");
    }

    return 0;
}

