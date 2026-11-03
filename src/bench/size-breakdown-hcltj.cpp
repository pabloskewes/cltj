// Serialized bytes of an H-CLTJ index, one line per trie with the bytes of every member
// (get_size_breakdown), plus the sdsl structure tree as JSON. Exits with 1 if the members
// do not add up to the serialized size. With the root bitmap, one more line with every bitmap
// and support of the root bitmap; root_block_bits is the block of the root in B (its degree),
// part of bv, which stays in B while its labels leave L_plain.
// Usage: size-breakdown-hcltj <index.hcltj> <out.json>
#include <CLI11.hpp>
#ifdef CLTJ_ROOT_BITMAP
#include <index/cltj_index_metatrie_hash_rb.hpp>
using index_type = cltj::compact_ltj_metatrie_hash_rb;
#else
#include <index/cltj_index_metatrie.hpp>
using index_type = cltj::compact_ltj_metatrie_hash;
#endif
#include <sdsl/io.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

static void print_policy(const cltj::overlay::SepBitmapOverlay& ov) {
    std::cout << " lsep_size=" << ov.lsep_size() << " lsep_ones=" << ov.lsep_ones()
              << " lsep_density=" << double(ov.lsep_ones()) / double(ov.lsep_size());
}

static void print_policy(const cltj::overlay::StepTableOverlay&) {}

static void print_policy(const cltj::overlay::StepTableRbOverlay& ov) { std::cout << " root_deleted=" << ov.root_deleted(); }

int main(int argc, char** argv) {
    CLI::App app{"Size breakdown of an H-CLTJ index"};
    std::string index_file, json_file;
    app.add_option("index", index_file, "H-CLTJ index file (.hcltj)")->required();
    app.add_option("out", json_file, "Output JSON with the sdsl structure tree")->required();
    CLI11_PARSE(app, argc, argv);

    using overlay_type = index_type::trie_type::overlay_type;
    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

    try {
        index_type index;
        if (!sdsl::load_from_file(index, index_file)) {
            std::cerr << "ERROR: cannot load " << index_file << std::endl;
            return 2;
        }
        const uint64_t index_bytes = sdsl::size_in_bytes(index);
        std::cout << "index_bytes=" << index_bytes << " format_version=" << int(overlay_type::format_version)
                  << std::endl;

        bool ok = true;
        uint64_t sum_tries = 0;
        uint64_t root_bytes = 0;
#ifdef CLTJ_ROOT_BITMAP
        uint64_t root_block_bits = 0;
#endif
        for (int t = 0; t < 6; t++) {
            const auto* trie = index.get_trie(t);
            const uint64_t trie_bytes = sdsl::size_in_bytes(*trie);
            const auto breakdown = trie->get_size_breakdown();
            uint64_t hashed_nodes = 0, hashed_keys = 0;
            for (uint64_t v = 0, deg = 0; v + 1 < trie->louds_size(); v += deg) {
                deg = trie->children(v);
                if (trie->node_has_hash(v)) {
                    hashed_nodes++;
                    hashed_keys += deg;
                }
            }
            std::cout << "trie=" << t << " name=" << names[t] << " bytes=" << trie_bytes
                      << " louds_size=" << trie->louds_size() << " n_labels=" << trie->louds_size() - 1
                      << " plain_size=" << trie->seq_plain().size()
                      << " plain_width=" << int(trie->seq_plain().width()) << " mphfs=" << trie->mphf_count()
                      << " hashed_nodes=" << hashed_nodes << " hashed_keys=" << hashed_keys;
            print_policy(trie->overlay());
#ifdef CLTJ_ROOT_BITMAP
            if (t % 2 == 0) {
                std::cout << " root_block_bits=" << trie->root_degree();
                root_block_bits += trie->root_degree();
            }
#endif
            for (const auto& c : breakdown.components)
                std::cout << " " << c.first << "=" << c.second;
            std::cout << " sum=" << breakdown.total_bytes() << std::endl;
            if (breakdown.total_bytes() != trie_bytes || hashed_nodes != trie->mphf_count()) {
                std::cerr << "ERROR: trie " << t << ": members add up to " << breakdown.total_bytes() << " of "
                          << trie_bytes << " bytes, " << hashed_nodes << " hashed nodes of " << trie->mphf_count()
                          << " MPHFs" << std::endl;
                ok = false;
            }
            sum_tries += trie_bytes;
        }
#ifdef CLTJ_ROOT_BITMAP
        root_bytes = sdsl::size_in_bytes(index.root());
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
#endif
        if (sum_tries + root_bytes != index_bytes) {
            std::cerr << "ERROR: tries and root bitmap add up to " << sum_tries + root_bytes << " of " << index_bytes
                      << " bytes" << std::endl;
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
