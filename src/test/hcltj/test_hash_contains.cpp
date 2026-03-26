// Verifies node_has_hash() and hash_contains() on compact_metatrie_hash.
// For each hashed node (>= threshold children): all children must return true,
// and a non-child key must return false.
// Runs with multiple thresholds {1, 2, 3} to cover different sets of hashed nodes.
// Uses data/data.txt.hcltj (11 triples).
#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <string>
#include <vector>

struct NodeData {
    uint64_t node_pos;
    uint64_t n_children;
};

struct TrieData {
    const char* name;
    std::vector<NodeData> all_nodes;  // every node with >= 1 child
};

int test_trie(cltj::compact_metatrie_hash* trie, const TrieData& data, uint32_t threshold) {
    int failures = 0;

    for (auto& [node_pos, n_children] : data.all_nodes) {
        bool should_be_hashed = (n_children >= threshold);
        bool is_hashed = trie->node_has_hash(node_pos);

        if (should_be_hashed != is_hashed) {
            std::cout << "  FAIL " << data.name << " node=" << node_pos << " node_has_hash()=" << is_hashed
                      << " expected=" << should_be_hashed << std::endl;
            failures++;
            continue;
        }

        if (!is_hashed)
            continue;

        bool all_true = true;
        for (uint64_t k = 0; k < n_children; k++) {
            uint32_t key = static_cast<uint32_t>(trie->seq[node_pos + k]);
            if (!trie->hash_contains(node_pos, key)) {
                std::cout << "  FAIL " << data.name << " node=" << node_pos << " hash_contains(" << key
                          << ") false (is a child)" << std::endl;
                all_true = false;
                failures++;
            }
        }
        if (all_true)
            std::cout << "  PASS " << data.name << " node=" << node_pos << " all " << n_children
                      << " children return true" << std::endl;

        uint32_t non_child = 999999;
        if (trie->hash_contains(node_pos, non_child)) {
            std::cout << "  FAIL " << data.name << " node=" << node_pos
                      << " hash_contains(999999) true (not a child)" << std::endl;
            failures++;
        } else {
            std::cout << "  PASS " << data.name << " node=" << node_pos
                      << " non-child 999999 correctly returns false" << std::endl;
        }
    }
    return failures;
}

int main(int argc, char** argv) {
    std::string index_path = "data/data.txt.hcltj";
    if (argc >= 2)
        index_path = argv[1];

    std::ifstream f(index_path);
    if (!f.good()) {
        std::cerr << "ERROR: index file not found: " << index_path << std::endl;
        std::cerr << "  Build it with: ./build/bench/build-hcltj data/data.txt" << std::endl;
        return 1;
    }
    f.close();

    cltj::compact_ltj_metatrie_hash index;
    sdsl::load_from_file(index, index_path);

    // All nodes with >= 1 child, from journal 04
    TrieData tries[6] = {
        {"SPO",
         {{0, 4},
         {4, 3},
         {7, 4},
         {11, 2},
         {13, 1},
         {14, 1},
         {15, 2},
         {17, 1},
         {18, 1},
         {19, 1},
         {20, 1},
         {21, 1},
         {22, 1},
         {23, 1},
         {24, 1}}                                                       },
        {"SOP",                         {{0, 4}, {4, 4}, {8, 1}, {9, 1}}},
        {"POS",
         {{0, 7},
         {7, 1},
         {8, 2},
         {10, 3},
         {13, 2},
         {15, 1},
         {16, 1},
         {17, 1},
         {18, 1},
         {19, 1},
         {20, 1},
         {21, 1},
         {22, 1},
         {23, 1},
         {24, 1},
         {25, 1},
         {26, 1},
         {27, 1},
         {28, 1}}                                                       },
        {"PSO", {{0, 1}, {1, 1}, {2, 3}, {5, 2}, {7, 1}, {8, 1}, {9, 1}}},
        {"OSP",
         {{0, 7},
         {7, 1},
         {8, 1},
         {9, 1},
         {10, 2},
         {12, 1},
         {13, 2},
         {15, 2},
         {17, 1},
         {18, 1},
         {19, 2},
         {21, 1},
         {22, 1},
         {23, 1},
         {24, 1},
         {25, 1},
         {26, 1},
         {27, 1}}                                                       },
        {"OPS", {{0, 1}, {1, 1}, {2, 2}, {4, 2}, {6, 1}, {7, 2}, {9, 2}}},
    };

    std::vector<uint32_t> thresholds = {1, 2, 3};
    int total_failures = 0;

    for (uint32_t threshold : thresholds) {
        std::cout << "\n=== threshold=" << threshold << " ===" << std::endl;
        for (int i = 0; i < 6; i++)
            index.get_trie(i)->build_hash_overlay(threshold);

        for (int t = 0; t < 6; t++)
            total_failures += test_trie(index.get_trie(t), tries[t], threshold);
    }

    std::cout << std::endl;
    if (total_failures == 0)
        std::cout << "ALL PASS" << std::endl;
    else
        std::cout << total_failures << " FAILED" << std::endl;

    return total_failures > 0 ? 1 : 0;
}
