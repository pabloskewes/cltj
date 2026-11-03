#include <CLI11.hpp>
#ifdef CLTJ_ROOT_BITMAP
#include <index/cltj_index_metatrie_hash_rb.hpp>
using index_type = cltj::compact_ltj_metatrie_hash_rb;
#else
#include <index/cltj_index_metatrie.hpp>
using index_type = cltj::compact_ltj_metatrie_hash;
#endif
#include <metatrie/overlay/checks.hpp>
#include <iostream>

int main(int argc, char** argv) {
    CLI::App app{"Check the structure, the key cursors and the translation overlay of an H-CLTJ index"};
    std::string index_file;
    app.add_option("index", index_file, "H-CLTJ index file (.hcltj)")->required();
    CLI11_PARSE(app, argc, argv);

    index_type index;
    if (!sdsl::load_from_file(index, index_file)) {
        std::cerr << "cannot load " << index_file << std::endl;
        return 1;
    }

    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
    bool ok = true;
    for (int i = 0; i < 6; i++) {
        const auto* trie = index.get_trie(i);
        auto labels = cltj::overlay::check_labels(*trie);
        auto policy = cltj::overlay::check_policy(*trie);
        std::cout << names[i] << " (mphfs=" << trie->mphf_count() << ", |L|=" << trie->louds_size() - 1
                  << ", |L_plain|=" << trie->seq_plain().size() << ")\n";
        cltj::overlay::print_report(std::cout, "structure", labels);
        cltj::overlay::print_report(std::cout, "policy", policy);
        std::cout << "  overlay bytes:";
        for (const auto& c : trie->overlay().get_size_breakdown().components)
            std::cout << " " << c.first << "=" << c.second;
        std::cout << "\n";
        ok = ok && labels.ok() && policy.ok();
    }
    std::cout << (ok ? "RESULT: ALL PASS" : "RESULT: FAIL") << std::endl;
    return ok ? 0 : 1;
}
