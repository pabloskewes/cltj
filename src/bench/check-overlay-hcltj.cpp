#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <metatrie/overlay/checks.hpp>
#include <iostream>

int main(int argc, char** argv) {
    CLI::App app{"Check L_plain and the translation overlay of an H-CLTJ index against m_seq"};
    std::string index_file;
    app.add_option("index", index_file, "H-CLTJ index file (.hcltj)")->required();
    CLI11_PARSE(app, argc, argv);

    cltj::compact_ltj_metatrie_hash index;
    if (!sdsl::load_from_file(index, index_file)) {
        std::cerr << "cannot load " << index_file << std::endl;
        return 1;
    }

    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
    bool ok = true;
    for (int i = 0; i < 6; i++) {
        const auto* trie = index.get_trie(i);
        auto dual = cltj::overlay::check_dual_phase(*trie);
        auto policy = cltj::overlay::check_policy(*trie);
        std::cout << names[i] << " (mphfs=" << trie->mphf_count() << ", |L|=" << trie->seq.size()
                  << ", |L_plain|=" << trie->seq_plain().size() << ")\n";
        cltj::overlay::print_report(std::cout, "dual phase", dual);
        cltj::overlay::print_report(std::cout, "policy", policy);
        std::cout << "  overlay bytes:";
        for (const auto& c : trie->overlay().get_size_breakdown().components)
            std::cout << " " << c.first << "=" << c.second;
        std::cout << "\n";
        ok = ok && dual.ok() && policy.ok();
    }
    std::cout << (ok ? "RESULT: ALL PASS" : "RESULT: FAIL") << std::endl;
    return ok ? 0 : 1;
}
