// Checks an X-CLTJ index with root bitmap against the X-CLTJ index of the same dataset:
//   - root: for every child i of the root of SPO, POS and OSP, the i-th id of the root bitmap
//     (walked with next) is the label X keeps at L[i], position() gives back i, and the bitmap
//     has no other ids;
//   - B: every trie has the same B;
//   - L: the L of every trie is the L of X without its first `offset` labels, and offset is the
//     degree of the root in a full trie and 0 in a partial one.
// Prints what differs (at most 5 labels per trie and check) and exits with 1 if anything does.
// Usage: check-root-bitmap-xcltj <index.xcltj> <index.xcltj-rb>
#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <index/cltj_index_metatrie_rb.hpp>
#include <sdsl/io.hpp>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

namespace {

template <class Trie> sdsl::bit_vector louds_of(const Trie& trie) {
    std::stringstream ss;
    trie.serialize(ss);
    sdsl::bit_vector bv;
    bv.load(ss);
    return bv;
}

const uint64_t max_reported = 5;

} // namespace

int main(int argc, char** argv) {
    CLI::App app{"Check the root bitmap and L of X-CLTJ with root bitmap against X-CLTJ"};
    std::string x_file, rb_file;
    app.add_option("x", x_file, "X-CLTJ index (.xcltj)")->required();
    app.add_option("rb", rb_file, "X-CLTJ index with root bitmap (.xcltj-rb)")->required();
    CLI11_PARSE(app, argc, argv);

    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

    try {
        cltj::compact_ltj_metatrie x;
        cltj::compact_ltj_metatrie_rb rb;
        if (!sdsl::load_from_file(x, x_file) || !sdsl::load_from_file(rb, rb_file)) {
            std::cerr << "ERROR: cannot load the indexes" << std::endl;
            return 2;
        }
        const auto& root = rb.root();
        bool ok = true;

        for (int t = 0; t < 6; t++) {
            const auto* xt = x.get_trie(t);
            const auto* rt = rb.get_trie(t);
            const auto& xseq = xt->seq;
            const auto& rseq = rt->base().seq;
            const bool full = (t % 2 == 0);
            const uint64_t deg = xt->root_degree();
            const uint64_t expected_offset = full ? deg : 0;
            uint64_t root_bad = 0, l_bad = 0;

            if (rt->offset() != expected_offset) {
                std::cerr << names[t] << ": offset " << rt->offset() << ", expected " << expected_offset << std::endl;
                ok = false;
            }
            if (louds_of(*xt) != louds_of(rt->base())) {
                std::cerr << names[t] << ": B differs" << std::endl;
                ok = false;
            }

            if (full) {
                const auto st = static_cast<state_type>(t / 2);
                if (root.count(st) != deg) {
                    std::cerr << names[t] << ": root bitmap has " << root.count(st) << " ids, root degree " << deg
                              << std::endl;
                    ok = false;
                }
                uint64_t c = root.next(st, 0);
                for (uint64_t i = 0; i < deg; ++i) {
                    if (c != xseq[i] || !root.contains(st, c) || root.position(st, c) != i) {
                        if (root_bad++ < max_reported)
                            std::cerr << names[t] << ": root child " << i << ": bitmap " << c << " (position "
                                      << root.position(st, c) << "), X " << xseq[i] << std::endl;
                    }
                    c = root.next(st, uint64_t(c) + 1);
                }
                if (c != 0) {
                    std::cerr << names[t] << ": root bitmap has ids after the last child, first " << c << std::endl;
                    ok = false;
                }
            }

            if (rseq.size() + expected_offset != xseq.size()) {
                std::cerr << names[t] << ": |L| " << rseq.size() << " + offset " << expected_offset << " != |L of X| "
                          << xseq.size() << std::endl;
                ok = false;
            } else {
                for (uint64_t i = 0; i < rseq.size(); ++i)
                    if (rseq[i] != xseq[i + expected_offset] && l_bad++ < max_reported)
                        std::cerr << names[t] << ": L[" << i << "] = " << rseq[i] << ", X L[" << i + expected_offset
                                  << "] = " << xseq[i + expected_offset] << std::endl;
            }

            std::cout << "trie=" << t << " name=" << names[t];
            if (full)
                std::cout << " root_degree=" << deg << " root_mismatches=" << root_bad;
            std::cout << " offset=" << rt->offset() << " labels=" << rseq.size() << " l_mismatches=" << l_bad
                      << std::endl;
            if (root_bad || l_bad)
                ok = false;
        }
        std::cout << (ok ? "RESULT: ROOT BITMAP AND L MATCH X" : "RESULT: FAIL") << std::endl;
        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    }
}
