#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/packed_trit.hpp>
#include <hashing/storage/glgh.hpp>
#include <hashing/key_policies.hpp>
#include <iomanip>
#include <sstream>
#include <vector>
#include <random>
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_set>

using cltj::hashing::BaselineStorage;
using cltj::hashing::CompressedBitvector;
using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::PackedTritStorage;
using cltj::hashing::policies::NoKey;
using cltj::hashing::policies::QuotientKey;

namespace {

template <typename Storage, typename Policy>
void analyze_strategy(
    const std::string& header,
    const std::vector<uint64_t>& keys,
    const std::string& g_label,
    const std::string& used_label,
    const std::string& rank_label
) {
    std::cout << "\n" << header << std::endl;
    MPHF<Storage, Policy> mphf;
    bool success = mphf.build(keys);
    if (!success) {
        std::cout << "Build failed!" << std::endl;
        return;
    }

    auto breakdown = mphf.get_size_breakdown();
    double bits_per_key = (breakdown.total_bytes() * 8.0) / keys.size();

    std::cout << "Size breakdown:" << std::endl;
    auto print_component = [&](const std::string& label, size_t bytes) {
        if (bytes == 0)
            return;
        std::cout << "  " << label << ": " << bytes << " bytes ("
                  << (bytes * 8.0 / keys.size()) << " bits/key)" << std::endl;
    };
    print_component(g_label, breakdown.g_bytes);
    print_component(used_label, breakdown.used_pos_bytes);
    print_component(rank_label, breakdown.rank_bytes);
    if (breakdown.q_bytes > 0) {
        print_component("Key payload", breakdown.q_bytes);
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << bits_per_key;
    std::cout << "  TOTAL: " << breakdown.total_bytes() << " bytes (" << oss.str() << " bits/key)"
              << std::endl;
}

}  // namespace

int main() {
    std::cout << "=== MPHF Size Breakdown Analysis ===" << std::endl;

    // Test with different sizes
    // std::vector<size_t> test_sizes = {1000, 10000, 100000};
    std::vector<size_t> test_sizes = {10000000};

    for (size_t n : test_sizes) {
        std::cout << "--- Analysis for n = " << n << " ---" << std::endl;

        // Generate test keys
        std::mt19937_64 rng(42);
        std::vector<uint64_t> keys;
        for (size_t i = 0; i < n; ++i) {
            keys.push_back(rng());
        }

        analyze_strategy<BaselineStorage, NoKey>(
            "[BaselineStorage]",
            keys,
            "G array (2-bit values)",
            "Used positions bitvector",
            "Rank support"
        );

        analyze_strategy<PackedTritStorage<CompressedBitvector>, NoKey>(
            "[PackedTritStorage (Compressed B)]",
            keys,
            "G' array (packed trits)",
            "Used positions bitvector",
            "Rank support"
        );

        analyze_strategy<GlGhStorage, NoKey>(
            "[GlGhStorage (Gl/Gh on-the-fly B)]",
            keys,
            "Gl + Gh bitvectors",
            "B stored explicitly",
            "Rank metadata (on-the-fly B)"
        );

        analyze_strategy<GlGhStorage, QuotientKey>(
            "[GlGhStorage + QuotientKey payload]",
            keys,
            "Gl + Gh bitvectors",
            "B stored explicitly",
            "Rank metadata (on-the-fly B)"
        );

        std::cout << std::endl;
    }

    return 0;
}
