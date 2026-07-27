// Inspects one hashed node of an H-CLTJ index.
// Contrasts the labels stored in m_seq against the keys the MPHF cursor
// reconstructs, reports duplicates on either side, and lists the slots that no
// key locates to.
// Usage: inspect-node-hcltj <path-to-index.hcltj> --trie <0-5> --node <louds-pos>
#include <index/cltj_index_metatrie.hpp>

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "CLI11.hpp"

static const char* trie_names[6] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
static constexpr uint64_t MAX_REPORTED = 10;

static void report_duplicates(const char* label, const std::vector<uint32_t>& values) {
    std::map<uint32_t, std::vector<uint64_t>> positions;
    for (uint64_t i = 0; i < values.size(); i++)
        positions[values[i]].push_back(i);

    uint64_t n_duplicated = 0;
    for (const auto& [key, at] : positions) {
        if (at.size() < 2)
            continue;
        n_duplicated++;
        if (n_duplicated <= MAX_REPORTED) {
            std::cout << "  " << label << " repeated key=" << key << " at";
            for (uint64_t i : at)
                std::cout << " " << i;
            std::cout << std::endl;
        }
    }

    std::cout << "  " << label << ": " << values.size() << " values, " << positions.size()
              << " distinct, " << n_duplicated << " repeated" << std::endl;
}

int main(int argc, char** argv) {
    CLI::App app{"Inspect one hashed node of an H-CLTJ index"};
    std::string index_file;
    uint32_t trie_id = 0;
    uint64_t node = 0;
    app.add_option("index", index_file, "Path to .hcltj index file")->required();
    app.add_option("--trie", trie_id, "Trie id (0=SPO, 1=SOP, 2=POS, 3=PSO, 4=OSP, 5=OPS)")->required();
    app.add_option("--node", node, "LOUDS position of the node")->required();
    CLI11_PARSE(app, argc, argv);

    if (trie_id > 5) {
        std::cerr << "ERROR: trie id must be in [0, 5]" << std::endl;
        return 1;
    }

    std::cout << "Loading index: " << index_file << std::endl;
    cltj::compact_ltj_metatrie_hash index;
    sdsl::load_from_file(index, index_file);
    std::cout << "Index loaded: " << sdsl::size_in_bytes(index) << " bytes" << std::endl;

    auto* trie = index.get_trie(trie_id);
    uint64_t degree = trie->children(node);

    std::cout << "\nTrie " << trie_names[trie_id] << " node=" << node << " degree=" << degree
              << " has_hash=" << (trie->node_has_hash(node) ? "yes" : "no")
              << " louds_size=" << trie->louds_size() << " root_degree=" << trie->root_degree()
              << std::endl;

    if (!trie->node_has_hash(node)) {
        std::cerr << "ERROR: node has no MPHF overlay" << std::endl;
        return 1;
    }

    std::vector<uint32_t> from_seq;
    from_seq.reserve(degree);
    for (uint64_t k = 0; k < degree; k++)
        from_seq.push_back(static_cast<uint32_t>(trie->seq[node + k]));

    std::vector<uint32_t> from_cursor;
    from_cursor.reserve(degree);
    for (auto cur = trie->hash_keys(node); cur.next();)
        from_cursor.push_back(cur.key());

    std::cout << "\n--- labels ---" << std::endl;
    report_duplicates("m_seq ", from_seq);
    report_duplicates("cursor", from_cursor);

    std::cout << "\n--- slot by slot ---" << std::endl;
    uint64_t compared = std::min(from_seq.size(), from_cursor.size());
    uint64_t n_mismatches = 0;
    for (uint64_t k = 0; k < compared; k++) {
        if (from_seq[k] == from_cursor[k])
            continue;
        n_mismatches++;
        if (n_mismatches <= MAX_REPORTED)
            std::cout << "  slot=" << k << " m_seq=" << from_seq[k] << " cursor=" << from_cursor[k]
                      << std::endl;
    }
    std::cout << "  compared " << compared << " slots, " << n_mismatches << " mismatches" << std::endl;

    std::cout << "\n--- slots reached by hash_locate ---" << std::endl;
    std::vector<uint8_t> reached(degree, 0);
    uint64_t not_found = 0;
    for (uint32_t key : from_seq) {
        auto [found, slot] = trie->hash_locate(node, key);
        if (!found) {
            not_found++;
            continue;
        }
        if (slot < degree)
            reached[slot] = 1;
    }

    uint64_t n_unreached = 0;
    for (uint64_t slot = 0; slot < degree; slot++) {
        if (reached[slot])
            continue;
        n_unreached++;
        if (n_unreached <= MAX_REPORTED)
            std::cout << "  slot=" << slot << " is not the image of any label" << std::endl;
    }
    std::cout << "  " << n_unreached << " unreached slots, " << not_found << " labels not found"
              << std::endl;

    std::cout << "\n--- first labels of the block ---" << std::endl;
    for (uint64_t k = 0; k < 5 && k < degree; k++)
        std::cout << "  m_seq[" << k << "]=" << from_seq[k] << " cursor[" << k << "]=" << from_cursor[k]
                  << std::endl;

    return 0;
}
