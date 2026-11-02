#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <metatrie/overlay/checks.hpp>
#include <iostream>

using index_a = cltj::cltj_index_metatrie<cltj::compact_metatrie_hash<cltj::overlay::SepBitmapOverlay>>;
using index_c = cltj::cltj_index_metatrie<cltj::compact_metatrie_hash<cltj::overlay::StepTableOverlay>>;

int main(int argc, char** argv) {
    CLI::App app{"Compare an H-CLTJ index built with scheme A against one built with scheme C from the same data"};
    std::string file_a, file_c;
    app.add_option("index_a", file_a, "Index built with scheme A (.hcltj)")->required();
    app.add_option("index_c", file_c, "Index built with scheme C (.hcltj)")->required();
    CLI11_PARSE(app, argc, argv);

    index_a a;
    index_c c;
    if (!sdsl::load_from_file(a, file_a) || !sdsl::load_from_file(c, file_c)) {
        std::cerr << "cannot load " << file_a << " or " << file_c << std::endl;
        return 1;
    }

    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
    bool ok = true;
    for (int i = 0; i < 6; i++) {
        auto rep = cltj::overlay::check_overlay_equal(*a.get_trie(i), *c.get_trie(i));
        std::cout << names[i] << " (mphfs=" << a.get_trie(i)->mphf_count() << ")\n";
        cltj::overlay::print_report(std::cout, "A vs C", rep);
        ok = ok && rep.ok();
    }
    std::cout << (ok ? "RESULT: ALL PASS" : "RESULT: FAIL") << std::endl;
    return ok ? 0 : 1;
}
