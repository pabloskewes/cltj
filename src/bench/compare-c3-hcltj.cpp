// Compares a C3 index (root bitmap, L_plain split by level) with the C2 one (root bitmap, a single
// width) built from the same data and threshold. Per trie:
//   - the serialization without the format tag and L_plain is the same bytes (B, supports,
//     threshold, overlay, MPHFs), and at every block start v has_dict, dict_idx and plain_start
//     (non-hashed v > 0) agree;
//   - |L_plain| is the same and C3 access(i) == C2 L_plain[i] for every i;
//   - n2 is the number of labels of the non-hashed level-1 nodes (|L_plain| in a partial trie),
//     the level-1 blocks lie in [0, n2) and the other ones in [n2, |L_plain|);
//   - with --seek, in every non-hashed block, binary_search_seek of C3 gives the same as C2 for the
//     label at each position k, searched from the block start and from k, and for that label + 1.
// The root bitmaps are compared byte by byte. Prints what differs (at most 5 per trie) and exits
// with 1 if anything does.
// Usage: compare-c3-hcltj <index C2-rb> <index C3> [--seek]
#include <CLI11.hpp>
#include <index/cltj_index_metatrie_hash_rb.hpp>
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

// Serialization of trie without the format tag and the L_plain member.
template <class Trie> std::string serialized_without_labels(const Trie& trie) {
    std::stringstream ss;
    sdsl::structure_tree_node root("root", "root");
    trie.serialize(ss, &root, "trie");
    const std::string bytes = ss.str();
    const sdsl::structure_tree_node* node = root.children.begin()->second.get();
    uint64_t before = 0, labels = 0;
    for (const auto& c : node->children) {
        const std::string& n = c.second->name;
        if (n == "bv")
            before += c.second->size;
        else if (n == "seq_plain")
            labels = c.second->size;
    }
    const uint64_t tag = sizeof(uint64_t);
    return bytes.substr(tag, before) + bytes.substr(tag + before + labels);
}

template <class T> std::string serialized(const T& x) {
    std::stringstream ss;
    x.serialize(ss);
    return ss.str();
}

} // namespace

int main(int argc, char** argv) {
    CLI::App app{"Compare a C3 index with the C2-rb index of the same data and threshold"};
    std::string c2_file, c3_file;
    bool seek = false;
    app.add_option("c2", c2_file, "H-CLTJ index with root bitmap (C2-rb)")->required();
    app.add_option("c3", c3_file, "C3 index, same data and threshold")->required();
    app.add_flag("--seek", seek, "Also compare binary_search_seek in every non-hashed block");
    CLI11_PARSE(app, argc, argv);

    try {
        cltj::compact_ltj_metatrie_hash_rb c2;
        cltj::compact_ltj_metatrie_hash_c3 c3;
        if (!sdsl::load_from_file(c2, c2_file)) {
            std::cerr << "ERROR: cannot load " << c2_file << std::endl;
            return 2;
        }
        if (!sdsl::load_from_file(c3, c3_file)) {
            std::cerr << "ERROR: cannot load " << c3_file << std::endl;
            return 2;
        }
        bool ok = true;
        if (serialized(c2.root()) != serialized(c3.root())) {
            std::cerr << "root bitmaps differ" << std::endl;
            ok = false;
        }

        for (int t = 0; t < 6; t++) {
            report rep{t};
            const auto& a = *c2.get_trie(t);
            const auto& b = *c3.get_trie(t);
            const auto& la = a.seq_plain();
            const auto& lb = b.seq_plain();
            const bool full = (t % 2 == 0);
            uint64_t seeks = 0;

            if (serialized_without_labels(a) != serialized_without_labels(b))
                rep.fail("serialization without L_plain differs (B, supports, threshold, overlay or MPHFs)");
            if (la.size() != lb.size())
                rep.fail("|L_plain| " + std::to_string(la.size()) + " vs " + std::to_string(lb.size()));

            for (uint64_t i = 0; i < la.size() && i < lb.size(); i++)
                if (la[i] != lb.access(i))
                    rep.fail("L_plain[" + std::to_string(i) + "]: C2 " + std::to_string(la[i]) + ", C3 " +
                             std::to_string(lb.access(i)));

            std::unordered_set<uint64_t> level1;
            if (full)
                for (uint64_t n = 1; n <= a.root_degree(); n++)
                    level1.insert(a.child(0, n));
            uint64_t level1_plain = 0;

            const uint64_t louds = a.louds_size();
            if (louds != b.louds_size())
                rep.fail("|B| differs");
            for (uint64_t v = 0, deg = 0; v + 1 < louds; v += deg) {
                deg = a.children(v);
                const bool hashed = a.node_has_hash(v);
                if (hashed != b.node_has_hash(v) || a.dict_idx(v) != b.dict_idx(v)) {
                    rep.fail("v=" + std::to_string(v) + ": has_dict or dict_idx differs");
                    continue;
                }
                if (hashed || (full && v == 0))
                    continue;
                const uint64_t s = a.overlay().plain_start(v);
                if (s != b.overlay().plain_start(v)) {
                    rep.fail("v=" + std::to_string(v) + ": plain_start differs");
                    continue;
                }
                const bool in_level1 = !full || level1.count(v);
                if (in_level1) {
                    level1_plain += deg;
                    if (s + deg > lb.n2())
                        rep.fail("v=" + std::to_string(v) + ": level-1 block ends after n2");
                } else if (s < lb.n2()) {
                    rep.fail("v=" + std::to_string(v) + ": level-2 block starts before n2");
                }

                if (!seek)
                    continue;
                const uint64_t off = a.deleted_before(v);
                const uint64_t end = v + deg - 1;
                auto same = [&](uint32_t val, uint64_t i) {
                    seeks++;
                    const auto pa = a.binary_search_seek(val, i, end, off);
                    const auto pb = b.binary_search_seek(val, i, end, b.deleted_before(v));
                    if (pa != pb)
                        rep.fail("v=" + std::to_string(v) + " seek(" + std::to_string(val) + ", " + std::to_string(i) +
                                 "): C2 {" + std::to_string(pa.first) + ", " + std::to_string(pa.second) + "}, C3 {" +
                                 std::to_string(pb.first) + ", " + std::to_string(pb.second) + "}");
                };
                for (uint64_t k = v; k <= end; k++) {
                    const uint32_t x = a.plain_label(k, off);
                    same(x, v);
                    same(x, k);
                    same(x + 1, v);
                }
            }
            if (level1_plain != lb.n2())
                rep.fail("n2 " + std::to_string(lb.n2()) + ", labels of non-hashed level-1 blocks " +
                         std::to_string(level1_plain));

            std::cout << "trie=" << t << " name=" << names[t] << " plain_size=" << lb.size()
                      << " c2_width=" << int(la.width()) << " n2=" << lb.n2() << " plain2_width=" << int(lb.plain2().width())
                      << " plain3_size=" << lb.plain3().size() << " plain3_width=" << int(lb.plain3().width())
                      << " mphfs=" << b.mphf_count() << " seeks=" << seeks << " errors=" << rep.errors << std::endl;
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
