// Serialized bytes of an X-CLTJ index with root bitmap: the format tag, one line per trie with
// the bytes of every member and the length and width of B and L, one line with every bitmap and
// support of the root bitmap, plus the sdsl structure tree as JSON. root_block_bits is the block of
// the root in B (its degree), which stays in B; it is part of bv, not an extra member. |B| is the
// first word of the serialized B, read from the file. Exits with 1 if the members do not add up.
// Under CLTJ_LEVEL_WIDTHS (index with one width per level of L), seq is split into plain2, plain3
// and n2, with the length and width of each level.
// Usage: size-breakdown-xcltj-rb <index.xcltj-rb> <out.json>
#include <CLI11.hpp>
#include <index/cltj_index_metatrie_rb.hpp>
#include <sdsl/io.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#ifdef CLTJ_LEVEL_WIDTHS
using index_type = cltj::compact_ltj_metatrie_rb_wg;
#else
using index_type = cltj::compact_ltj_metatrie_rb;
#endif

int main(int argc, char** argv) {
    CLI::App app{"Size breakdown of an X-CLTJ index with root bitmap"};
    std::string index_file, json_file;
    app.add_option("index", index_file, "X-CLTJ index with root bitmap (.xcltj-rb)")->required();
    app.add_option("out", json_file, "Output JSON with the sdsl structure tree")->required();
    CLI11_PARSE(app, argc, argv);

    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

    try {
        index_type index;
        if (!sdsl::load_from_file(index, index_file)) {
            std::cerr << "ERROR: cannot load " << index_file << std::endl;
            return 2;
        }

        const uint64_t index_bytes = sdsl::size_in_bytes(index);
        const uint64_t tag_bytes = sizeof(index_type::format_tag);
        std::cout << "index_bytes=" << index_bytes << " format_tag=" << tag_bytes;
#ifdef CLTJ_LEVEL_WIDTHS
        std::cout << " format_version=" << int(index_type::format_version);
#endif
        std::cout << std::endl;

        std::ifstream file(index_file, std::ios::binary);
        bool ok = true;
        uint64_t offset = tag_bytes, sum_tries = 0, root_block_bits = 0;
        for (int t = 0; t < 6; t++) {
            const auto* trie = index.get_trie(t);
            const uint64_t trie_bytes = sdsl::size_in_bytes(*trie);
            sdsl::structure_tree_node node("trie", "trie");
            sdsl::nullstream trie_ns;
            trie->serialize(trie_ns, &node, "trie");
            const sdsl::structure_tree_node* trie_node = nullptr;
            for (const auto& c : node.children.begin()->second->children)
                if (c.second->name == "trie")
                    trie_node = c.second.get();

            // In serialization order; a member missing here makes the sum fall short.
            std::vector<std::pair<std::string, uint64_t>> members;
            uint64_t sum = 0;
            for (const char* name : {"bv", "seq", "succ0", "select0"}) {
                uint64_t size = 0;
                for (const auto& c : trie_node->children)
                    if (c.second->name == name)
                        size = c.second->size;
                members.push_back({name, size});
                sum += size;
            }
            members.push_back({"offset", sizeof(trie->offset())});
            sum += sizeof(trie->offset());

            uint64_t b_bits = 0;
            file.seekg(offset);
            file.read(reinterpret_cast<char*>(&b_bits), sizeof(b_bits));
            const uint64_t bv_bytes = members[0].second;
            if (!file || bv_bytes != sizeof(uint64_t) * (1 + (b_bits + 63) / 64)) {
                std::cerr << "ERROR: trie " << t << ": B of " << b_bits << " bits read at offset " << offset
                          << " does not match bv=" << bv_bytes << " bytes" << std::endl;
                ok = false;
            }

            const auto& seq = trie->labels();
            std::cout << "trie=" << t << " name=" << names[t] << " bytes=" << trie_bytes << " louds_size=" << b_bits
                      << " seq_size=" << seq.size();
#ifdef CLTJ_LEVEL_WIDTHS
            std::cout << " plain2_size=" << seq.plain2().size() << " plain2_width=" << int(seq.plain2().width())
                      << " plain3_size=" << seq.plain3().size() << " plain3_width=" << int(seq.plain3().width());
#else
            std::cout << " seq_width=" << int(seq.width());
#endif
            std::cout << " label_offset=" << trie->offset();
            if (t % 2 == 0) {
                std::cout << " root_block_bits=" << trie->root_degree();
                root_block_bits += trie->root_degree();
            }
            for (const auto& m : members)
                std::cout << " " << m.first << "=" << m.second;
#ifdef CLTJ_LEVEL_WIDTHS
            for (const auto& c : seq.get_size_breakdown().components)
                std::cout << " " << c.first << "=" << c.second;
#endif
            std::cout << " sum=" << sum << std::endl;
            if (sum != trie_bytes) {
                std::cerr << "ERROR: trie " << t << ": members add up to " << sum << " of " << trie_bytes << " bytes"
                          << std::endl;
                ok = false;
            }
            offset += trie_bytes;
            sum_tries += trie_bytes;
        }

        const uint64_t root_bytes = sdsl::size_in_bytes(index.root());
        uint64_t root_sum = 0;
        std::cout << "root bytes=" << root_bytes;
        for (const auto& c : index.root().size_components()) {
            std::cout << " " << c.first << "=" << c.second;
            root_sum += c.second;
        }
        for (int st = 0; st < 3; st++)
            std::cout << " " << "spo"[st] << ".universe=" << index.root().universe(static_cast<state_type>(st))
                      << " " << "spo"[st] << ".count=" << index.root().count(static_cast<state_type>(st));
        std::cout << " sum=" << root_sum << std::endl;
        if (root_sum != root_bytes) {
            std::cerr << "ERROR: root bitmap members add up to " << root_sum << " of " << root_bytes << " bytes"
                      << std::endl;
            ok = false;
        }
        std::cout << "root_block_bits_in_b=" << root_block_bits << std::endl;

        if (tag_bytes + sum_tries + root_bytes != index_bytes) {
            std::cerr << "ERROR: tag, tries and root bitmap add up to " << tag_bytes + sum_tries + root_bytes << " of "
                      << index_bytes << " bytes" << std::endl;
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
