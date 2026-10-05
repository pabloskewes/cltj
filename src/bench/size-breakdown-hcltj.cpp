// LTJ-26 (throwaway): sdsl structure tree of an H-CLTJ index as JSON, plus root info per trie.
#include <fstream>
#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <sdsl/io.hpp>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <index.hcltj> <out.json>" << std::endl;
        return 1;
    }
    cltj::compact_ltj_metatrie_hash index;
    sdsl::load_from_file(index, argv[1]);
    std::cout << "index_bytes=" << sdsl::size_in_bytes(index) << std::endl;
    for (int t = 0; t < 6; t++) {
        auto* trie = index.get_trie(t);
        std::cout << "trie=" << t << " root_degree=" << trie->root_degree()
                  << " root_hashed=" << trie->node_has_hash(0) << " mphfs=" << trie->mphf_count()
                  << " louds_size=" << trie->louds_size() << " seq_width=" << (int)trie->seq.width()
                  << " seq_size=" << trie->seq.size();
        uint64_t hashed_nodes = 0, hashed_keys = 0, max_deg = 0;
        for (uint64_t pos = 0; pos + 1 < trie->louds_size(); ++pos) {
            if (trie->node_has_hash(pos)) {
                uint64_t d = trie->children(pos);
                ++hashed_nodes;
                hashed_keys += d;
                max_deg = std::max(max_deg, d);
            }
        }
        std::cout << " hashed_nodes=" << hashed_nodes << " hashed_keys=" << hashed_keys
                  << " max_hashed_degree=" << max_deg << std::endl;
    }
    std::ofstream out(argv[2]);
    sdsl::write_structure<sdsl::JSON_FORMAT>(index, out);
    return 0;
}
