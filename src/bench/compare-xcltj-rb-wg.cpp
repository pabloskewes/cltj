// Compares an X-CLTJ index with root bitmap and one width per level of L (X-rb-wg) with the X-rb
// index of the same data. Per trie:
//   - B and its supports are the same bytes, and so is offset;
//   - |L| is the same and wg access(i) == X-rb access(i) for every i;
//   - n2 is the number of labels of the level-1 blocks (|L| in a partial trie, where every block is
//     level 1), the level-1 blocks lie in [0, n2) and the other ones in [n2, |L|);
//   - with --seek, in every block, binary_search_seek of X-rb-wg gives the same as X-rb for the
//     label at each position k, searched from the block start and from k, and for that label + 1.
// The root bitmaps are compared byte by byte. Prints what differs (at most 5 per trie) and exits
// with 1 if anything does.
// Usage: compare-xcltj-rb-wg <index X-rb> <index X-rb-wg> [--seek]
#include <CLI11.hpp>
#include <index/cltj_index_metatrie_rb.hpp>
#include <sdsl/io.hpp>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

namespace {

const uint64_t max_reported = 5;
const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

struct report {
    int trie;
    uint64_t errors = 0;

    void fail(const std::string& msg) {
        if (errors++ < max_reported)
            std::cerr << names[trie] << ": " << msg << std::endl;
    }
};

template <class T> std::string serialized(const T& x) {
    std::stringstream ss;
    x.serialize(ss);
    return ss.str();
}

} // namespace

int main(int argc, char** argv) {
    CLI::App app{"Compare an X-rb-wg index with the X-rb index of the same data"};
    std::string xrb_file, wg_file;
    bool seek = false;
    app.add_option("xrb", xrb_file, "X-CLTJ index with root bitmap (X-rb)")->required();
    app.add_option("wg", wg_file, "X-rb-wg index, same data")->required();
    app.add_flag("--seek", seek, "Also compare binary_search_seek in every block");
    CLI11_PARSE(app, argc, argv);

    try {
        cltj::compact_ltj_metatrie_rb xrb;
        cltj::compact_ltj_metatrie_rb_wg wg;
        if (!sdsl::load_from_file(xrb, xrb_file)) {
            std::cerr << "ERROR: cannot load " << xrb_file << std::endl;
            return 2;
        }
        if (!sdsl::load_from_file(wg, wg_file)) {
            std::cerr << "ERROR: cannot load " << wg_file << std::endl;
            return 2;
        }
        bool ok = true;
        if (serialized(xrb.root()) != serialized(wg.root())) {
            std::cerr << "root bitmaps differ" << std::endl;
            ok = false;
        }

        for (int t = 0; t < 6; t++) {
            report rep{t};
            const auto& a = *xrb.get_trie(t);
            const auto& b = *wg.get_trie(t);
            const auto& la = a.labels();
            const auto& lb = b.labels();
            const bool full = (t % 2 == 0);
            const uint64_t off = a.offset();
            uint64_t seeks = 0;

            // base() holds B and its supports with an empty L.
            const std::string base_a = serialized(a.base());
            if (base_a != serialized(b.base()))
                rep.fail("B or its supports differ");
            if (off != b.offset())
                rep.fail("offset " + std::to_string(off) + " vs " + std::to_string(b.offset()));
            if (la.size() != lb.size())
                rep.fail("|L| " + std::to_string(la.size()) + " vs " + std::to_string(lb.size()));

            for (uint64_t i = 0; i < la.size() && i < lb.size(); i++)
                if (la.access(i) != lb.access(i))
                    rep.fail("L[" + std::to_string(i) + "]: X-rb " + std::to_string(la.access(i)) + ", wg " +
                             std::to_string(lb.access(i)));

            std::stringstream bs(base_a);
            sdsl::bit_vector bv;
            bv.load(bs);
            const uint64_t louds = bv.size();
            if (off + la.size() != louds)
                rep.fail("offset + |L| = " + std::to_string(off + la.size()) + ", |B| = " + std::to_string(louds));

            std::unordered_set<uint64_t> level1;
            if (full)
                for (uint64_t n = 1; n <= a.root_degree(); n++)
                    level1.insert(a.child(0, n));
            uint64_t level1_labels = 0, blocks = 0;

            for (uint64_t v = 0, deg = 0; v + 1 < louds; v += deg) {
                deg = a.children(v);
                if (full && v == 0)
                    continue;
                blocks++;
                const uint64_t s = v - off;
                if (!full || level1.count(v)) {
                    level1_labels += deg;
                    if (s + deg > lb.n2())
                        rep.fail("v=" + std::to_string(v) + ": level-1 block ends after n2");
                } else if (s < lb.n2()) {
                    rep.fail("v=" + std::to_string(v) + ": level-2 block starts before n2");
                }

                if (!seek)
                    continue;
                const uint64_t end = v + deg - 1;
                auto same = [&](uint32_t val, uint64_t i) {
                    seeks++;
                    const auto pa = a.binary_search_seek(val, i, end);
                    const auto pb = b.binary_search_seek(val, i, end);
                    if (pa != pb)
                        rep.fail("v=" + std::to_string(v) + " seek(" + std::to_string(val) + ", " + std::to_string(i) +
                                 "): X-rb {" + std::to_string(pa.first) + ", " + std::to_string(pa.second) + "}, wg {" +
                                 std::to_string(pb.first) + ", " + std::to_string(pb.second) + "}");
                };
                for (uint64_t k = v; k <= end; k++) {
                    const uint32_t x = a.label(k);
                    same(x, v);
                    same(x, k);
                    same(x + 1, v);
                }
            }
            // L ends with a label in no block, kept by X: level 3 in a full trie, level 2 in a partial one.
            const uint64_t expected_n2 = full ? level1_labels : level1_labels + 1;
            if (lb.n2() != expected_n2)
                rep.fail("n2 " + std::to_string(lb.n2()) + ", expected " + std::to_string(expected_n2));

            std::cout << "trie=" << t << " name=" << names[t] << " labels=" << lb.size()
                      << " xrb_width=" << int(la.width()) << " n2=" << lb.n2() << " plain2_width=" << int(lb.plain2().width())
                      << " plain3_size=" << lb.plain3().size() << " plain3_width=" << int(lb.plain3().width())
                      << " blocks=" << blocks << " seeks=" << seeks << " errors=" << rep.errors << std::endl;
            if (rep.errors)
                ok = false;
        }
        std::cout << (ok ? "RESULT: SAME" : "RESULT: DIFFERENT") << std::endl;
        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    }
}
