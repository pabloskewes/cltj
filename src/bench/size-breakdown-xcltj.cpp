// Serialized bytes of an X-CLTJ index, one line per trie with the bytes of every member and
// the length and width of B and L, plus the sdsl structure tree as JSON. |B| is the first
// word of the serialized B, read from the file. Exits with 1 if the members do not add up.
// Usage: size-breakdown-xcltj <index.xcltj> <out.json>
#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <sdsl/io.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char** argv) {
    CLI::App app{"Size breakdown of an X-CLTJ index"};
    std::string index_file, json_file;
    app.add_option("index", index_file, "X-CLTJ index file (.xcltj)")->required();
    app.add_option("out", json_file, "Output JSON with the sdsl structure tree")->required();
    CLI11_PARSE(app, argc, argv);

    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

    try {
        cltj::compact_ltj_metatrie index;
        if (!sdsl::load_from_file(index, index_file)) {
            std::cerr << "ERROR: cannot load " << index_file << std::endl;
            return 2;
        }

        const uint64_t index_bytes = sdsl::size_in_bytes(index);
        std::cout << "index_bytes=" << index_bytes << std::endl;

        std::ifstream file(index_file, std::ios::binary);
        bool ok = true;
        uint64_t offset = 0, sum_tries = 0;
        for (int t = 0; t < 6; t++) {
            const auto* trie = index.get_trie(t);
            const uint64_t trie_bytes = sdsl::size_in_bytes(*trie);
            sdsl::structure_tree_node node("trie", "trie");
            sdsl::nullstream trie_ns;
            trie->serialize(trie_ns, &node, "trie");
            const auto* trie_node = node.children.begin()->second.get();

            std::vector<std::pair<std::string, uint64_t>> members;
            uint64_t sum = 0;
            for (const auto& c : trie_node->children) {
                members.push_back({c.second->name, c.second->size});
                sum += c.second->size;
            }

            uint64_t b_bits = 0;
            file.seekg(offset);
            file.read(reinterpret_cast<char*>(&b_bits), sizeof(b_bits));
            uint64_t bv_bytes = 0;
            for (const auto& m : members)
                if (m.first == "bv")
                    bv_bytes = m.second;
            if (!file || bv_bytes != sizeof(uint64_t) * (1 + (b_bits + 63) / 64)) {
                std::cerr << "ERROR: trie " << t << ": B of " << b_bits << " bits read at offset " << offset
                          << " does not match bv=" << bv_bytes << " bytes" << std::endl;
                ok = false;
            }

            std::cout << "trie=" << t << " name=" << names[t] << " bytes=" << trie_bytes << " louds_size=" << b_bits
                      << " seq_size=" << trie->seq.size() << " seq_width=" << int(trie->seq.width());
            for (const auto& m : members)
                std::cout << " " << m.first << "=" << m.second;
            std::cout << " sum=" << sum << std::endl;
            if (sum != trie_bytes) {
                std::cerr << "ERROR: trie " << t << ": members add up to " << sum << " of " << trie_bytes << " bytes"
                          << std::endl;
                ok = false;
            }
            offset += trie_bytes;
            sum_tries += trie_bytes;
        }
        if (sum_tries != index_bytes) {
            std::cerr << "ERROR: tries add up to " << sum_tries << " of " << index_bytes << " bytes" << std::endl;
            ok = false;
        }

        std::ofstream out(json_file);
        sdsl::write_structure<sdsl::JSON_FORMAT>(index, out);
        if (!out) {
            std::cerr << "ERROR: cannot write " << json_file << std::endl;
            return 2;
        }
        std::cout << (ok ? "RESULT: SIZES ADD UP" : "RESULT: FAIL") << std::endl;
        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    }
}
