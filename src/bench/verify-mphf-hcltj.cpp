// Verifies MPHF correctness post-deserialization for an H-CLTJ index.
// BFS-walks all 6 tries. For every hashed node:
//   - the key cursor must emit deg(node) keys with slots 0, 1, ... (zero cursor mismatches)
//   - every emitted key must return hash_contains() == true and hash_locate() == {true, its
//     slot} (zero false negatives)
//   - sampled non-members must return hash_contains() == false (zero false positives)
// Usage: verify-mphf-hcltj <path-to-index.hcltj>
#ifdef CLTJ_ROOT_BITMAP
#include <index/cltj_index_metatrie_hash_rb.hpp>
using index_type = cltj::compact_ltj_metatrie_hash_rb;
#else
#include <index/cltj_index_metatrie.hpp>
using index_type = cltj::compact_ltj_metatrie_hash;
#endif

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

static const char* trie_names[6] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
static constexpr int NON_MEMBER_SAMPLES = 10;

struct TrieStats {
    uint64_t nodes_checked = 0;
    uint64_t keys_checked = 0;
    uint64_t non_member_checked = 0;
    uint64_t false_negatives = 0;
    uint64_t false_positives = 0;
    uint64_t cursor_keys_checked = 0;
    uint64_t cursor_mismatches = 0;
};

template <class Trie> static TrieStats verify_trie(Trie* trie, const char* name) {
    TrieStats stats;

    uint64_t bv_size = trie->louds_size();

    // num_zeros = number of internal LOUDS nodes (each terminated by a 0-bit).
    // Computed via children_histogram which safely walks the BFS internally.
    auto hist = trie->children_histogram();
    uint64_t num_zeros = 0;
    for (auto& [deg, cnt] : hist)
        num_zeros += cnt;

    std::mt19937 rng(42);
    std::vector<uint64_t> current_level = {0};

    while (!current_level.empty()) {
        std::vector<uint64_t> next_level;
        for (auto node : current_level) {
            uint64_t n_children = trie->children(node);

            if (trie->node_has_hash(node)) {
                stats.nodes_checked++;

                std::vector<uint32_t> children;
                children.reserve(n_children);
                uint32_t max_key = 0;
                uint64_t emitted = 0;
                for (auto cur = trie->hash_keys(node); cur.next(); emitted++) {
                    stats.cursor_keys_checked++;
                    if (cur.slot() != emitted) {
                        stats.cursor_mismatches++;
                        if (stats.cursor_mismatches <= 10)
                            std::cerr << "  CURSOR " << name << " node=" << node
                                      << " slot=" << cur.slot() << " expected_slot=" << emitted
                                      << " key=" << cur.key() << std::endl;
                    }
                    uint32_t key = cur.key();
                    children.push_back(key);
                    if (key > max_key) max_key = key;

                    stats.keys_checked++;
                    auto [found, slot] = trie->hash_locate(node, key);
                    if (!trie->hash_contains(node, key) || !found || slot != cur.slot()) {
                        stats.false_negatives++;
                        if (stats.false_negatives <= 10)
                            std::cerr << "  FN " << name << " node=" << node << " key=" << key
                                      << " (child, slot " << cur.slot() << ") locates to (" << found
                                      << ", " << slot << ")" << std::endl;
                    }
                }
                if (emitted != n_children) {
                    stats.cursor_mismatches++;
                    std::cerr << "  CURSOR " << name << " node=" << node << " emitted " << emitted
                              << " keys, expected " << n_children << std::endl;
                }
                std::sort(children.begin(), children.end());
                auto is_child = [&](uint32_t key) {
                    return std::binary_search(children.begin(), children.end(), key);
                };

                std::set<uint32_t> non_members;
                non_members.insert(max_key + 1);
                if (max_key + 2 <= UINT32_MAX)
                    non_members.insert(max_key + 2);

                std::uniform_int_distribution<uint32_t> dist(0, std::max(max_key * 2, uint32_t(1000)));
                while (static_cast<int>(non_members.size()) < NON_MEMBER_SAMPLES) {
                    uint32_t v = dist(rng);
                    if (!is_child(v))
                        non_members.insert(v);
                }

                for (uint32_t v : non_members) {
                    stats.non_member_checked++;
                    if (trie->hash_contains(node, v)) {
                        stats.false_positives++;
                        if (stats.false_positives <= 10)
                            std::cerr << "  FP " << name << " node=" << node
                                      << " key=" << v << " (not a child)" << std::endl;
                    }
                }
            }

            for (uint64_t n = 1; n <= n_children; n++) {
                if (node + 1 + n > num_zeros)
                    break;
                uint64_t child_node = trie->child(node, n);
                if (child_node + 1 < bv_size)
                    next_level.push_back(child_node);
            }
        }
        current_level = std::move(next_level);
    }

    return stats;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-index.hcltj>" << std::endl;
        return 1;
    }

    std::string index_path = argv[1];
    std::ifstream f(index_path);
    if (!f.good()) {
        std::cerr << "ERROR: index file not found: " << index_path << std::endl;
        return 1;
    }
    f.close();

    std::cout << "Loading index: " << index_path << std::endl;
    index_type index;
    sdsl::load_from_file(index, index_path);
    std::cout << "Index loaded: " << sdsl::size_in_bytes(index) << " bytes" << std::endl;

    uint64_t total_fn = 0, total_fp = 0, total_cursor_mismatches = 0;

    for (int t = 0; t < 6; t++) {
        std::cout << "\n--- Trie " << trie_names[t] << " ---" << std::endl;
        TrieStats s = verify_trie(index.get_trie(t), trie_names[t]);
        std::cout << "  hashed nodes:      " << s.nodes_checked << std::endl;
        std::cout << "  member checks:     " << s.keys_checked << std::endl;
        std::cout << "  non-member checks: " << s.non_member_checked << std::endl;
        std::cout << "  cursor keys:       " << s.cursor_keys_checked << std::endl;
        std::cout << "  false negatives:   " << s.false_negatives << std::endl;
        std::cout << "  false positives:   " << s.false_positives << std::endl;
        std::cout << "  cursor mismatches: " << s.cursor_mismatches << std::endl;
        total_fn += s.false_negatives;
        total_fp += s.false_positives;
        total_cursor_mismatches += s.cursor_mismatches;
    }

    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total false negatives: " << total_fn << std::endl;
    std::cout << "Total false positives: " << total_fp << std::endl;
    std::cout << "Total cursor mismatches: " << total_cursor_mismatches << std::endl;

    if (total_fn == 0 && total_fp == 0 && total_cursor_mismatches == 0) {
        std::cout << "RESULT: ALL PASS" << std::endl;
        return 0;
    } else {
        std::cout << "RESULT: FAILURES DETECTED" << std::endl;
        return 1;
    }
}
