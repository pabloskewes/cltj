// Verifies children_histogram() for all 6 tries of compact_ltj_metatrie_hash.
// Expected values derived from data/data.txt.hcltj (11 triples, small dataset).
// See journals/0016-embed-glgh-into-cltj/04-child-number-exp-results-small-dataset.md
#include <index/cltj_index_metatrie.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using histogram = std::map<uint64_t, uint64_t>;

bool check(const std::string& label, const histogram& got, const histogram& expected) {
    if (got == expected) {
        std::cout << "  PASS " << label << std::endl;
        return true;
    }
    std::cout << "  FAIL " << label << std::endl;
    std::cout << "    expected: ";
    for (auto& [k, v] : expected)
        std::cout << "{" << k << ":" << v << "} ";
    std::cout << std::endl;
    std::cout << "    got:      ";
    for (auto& [k, v] : got)
        std::cout << "{" << k << ":" << v << "} ";
    std::cout << std::endl;
    return false;
}

int main(int argc, char** argv) {
    std::string index_path = "data/data.txt.hcltj";
    if (argc >= 2)
        index_path = argv[1];

    std::ifstream f(index_path);
    if (!f.good()) {
        std::cerr << "ERROR: index file not found: " << index_path << std::endl;
        std::cerr << "  Build it first with: ./build/bench/build-hcltj data/data.txt" << std::endl;
        return 1;
    }
    f.close();

    cltj::compact_ltj_metatrie_hash index;
    sdsl::load_from_file(index, index_path);

    // Expected histograms from data/data.txt.hcltj
    const histogram expected[6] = {
        {{1, 10}, {2, 2}, {3, 1}, {4, 2}}, // SPO
        {{1, 2}, {4, 2}}, // SOP
        {{1, 15}, {2, 2}, {3, 1}, {7, 1}}, // POS
        {{1, 5}, {2, 1}, {3, 1}}, // PSO
        {{1, 13}, {2, 4}, {7, 1}}, // OSP
        {{1, 3}, {2, 4}}, // OPS
    };
    const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

    std::cout << "test_children_histogram (data/data.txt.hcltj)" << std::endl;
    int failures = 0;
    for (int t = 0; t < 6; t++) {
        auto hist = index.get_trie(t)->children_histogram();
        if (!check(names[t], hist, expected[t]))
            failures++;
    }

    if (failures == 0)
        std::cout << "ALL PASS" << std::endl;
    else
        std::cout << failures << " FAILED" << std::endl;

    return failures > 0 ? 1 : 0;
}
