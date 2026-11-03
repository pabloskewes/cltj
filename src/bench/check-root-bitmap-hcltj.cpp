// Checks an H-CLTJ index with root bitmap (scheme C) against the X-CLTJ and the H-CLTJ (scheme C)
// indexes of the same dataset, the H one with the same threshold:
//   - root: for every child i of the root of SPO, POS and OSP, the i-th id of the root bitmap
//     (walked with next) is the label X keeps at L[i], position() gives back i, and the bitmap has no
//     other ids; the root is unhashed and its block is the one left out of L_plain (root_deleted is
//     deg(root) in the full tries and 0 in the partial ones);
//   - below the root: every node has the same degree, is hashed or not, and has the same set of
//     labels (L_plain or key cursor) as the node of H reached by the same path of labels. Nodes are
//     matched by path and not by position: in H the children of a hashed root come in slot order;
//   - coverage: the matched blocks hold all the labels of B.
// Loads the index with root bitmap and X first, and frees X before loading H. Prints what differs
// (at most 5 per trie) and exits with 1 if anything does.
// Usage: check-root-bitmap-hcltj <index.xcltj> <index.hcltj> <index with root bitmap>
#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <index/cltj_index_metatrie_hash_rb.hpp>
#include <sdsl/io.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

const uint64_t max_reported = 5;
const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

struct trie_report {
    uint64_t blocks = 0;
    uint64_t labels = 0;
    uint64_t hashed_blocks = 0;
    uint64_t errors = 0;

    void fail(int t, const std::string& msg) {
        if (errors++ < max_reported)
            std::cerr << names[t] << ": " << msg << std::endl;
    }
};

// Labels of the block of v, in position order.
template <class Trie> void block_labels(const Trie& trie, uint64_t v, std::vector<uint32_t>& out) {
    const uint64_t deg = trie.children(v);
    out.resize(deg);
    if (trie.node_has_hash(v)) {
        auto cur = trie.hash_keys(v);
        while (cur.next())
            out[cur.slot()] = cur.key();
        return;
    }
    const uint64_t off = trie.deleted_before(v);
    for (uint64_t k = 0; k < deg; ++k)
        out[k] = trie.plain_label(v + k, off);
}

// Offsets of the block sorted by label.
void sorted_offsets(const std::vector<uint32_t>& labels, std::vector<uint32_t>& idx) {
    idx.resize(labels.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](uint32_t a, uint32_t b) { return labels[a] < labels[b]; });
}

template <class TrieR, class TrieH> struct block_matcher {
    const TrieR& r;
    const TrieH& h;
    int t;
    trie_report& rep;
    std::vector<uint32_t> lr, lh, ir, ih;

    // Compares the block of vr in r with the block of vh in h. With children, appends the pairs of
    // positions (in r, in h) of the same label.
    bool compare(uint64_t vr, uint64_t vh, std::vector<std::pair<uint64_t, uint64_t>>* children) {
        const uint64_t deg = r.children(vr);
        const bool hashed = r.node_has_hash(vr);
        rep.blocks++;
        rep.labels += deg;
        rep.hashed_blocks += hashed;
        if (h.children(vh) != deg || h.node_has_hash(vh) != hashed) {
            std::ostringstream m;
            m << "node " << vr << ": deg " << deg << " hashed " << hashed << ", in H node " << vh << ": deg "
              << h.children(vh) << " hashed " << h.node_has_hash(vh);
            rep.fail(t, m.str());
            return false;
        }
        block_labels(r, vr, lr);
        block_labels(h, vh, lh);
        if (hashed) {
            sorted_offsets(lr, ir);
            sorted_offsets(lh, ih);
        } else {
            ir.resize(deg);
            std::iota(ir.begin(), ir.end(), 0);
            ih = ir;
        }
        for (uint64_t k = 0; k < deg; ++k) {
            if (lr[ir[k]] != lh[ih[k]] || (k > 0 && lr[ir[k]] <= lr[ir[k - 1]])) {
                std::ostringstream m;
                m << "node " << vr << " (H node " << vh << "): " << k << "-th smallest label " << lr[ir[k]]
                  << ", in H " << lh[ih[k]];
                rep.fail(t, m.str());
                return false;
            }
        }
        if (children)
            for (uint64_t k = 0; k < deg; ++k)
                children->push_back({vr + ir[k], vh + ih[k]});
        return true;
    }
};

// Position of label x in the root block of h (hashed or not), or deg(root) if it is not there.
template <class TrieH> uint64_t root_position(const TrieH& h, uint32_t x) {
    const uint64_t deg = h.root_degree();
    if (h.node_has_hash(0)) {
        auto [found, slot] = h.hash_locate(0, x);
        return found ? slot : deg;
    }
    auto p = h.binary_search_seek(x, 0, deg - 1, h.deleted_before(0));
    return (p.second < deg && p.first == x) ? p.second : deg;
}

} // namespace

int main(int argc, char** argv) {
    CLI::App app{"Check the root bitmap of H-CLTJ (scheme C) against X-CLTJ, and the rest of the tries against H-CLTJ"};
    std::string x_file, h_file, rb_file;
    app.add_option("xcltj", x_file, "X-CLTJ index (.xcltj)")->required();
    app.add_option("hcltj", h_file, "H-CLTJ index, scheme C, same threshold")->required();
    app.add_option("rb", rb_file, "H-CLTJ index with root bitmap")->required();
    CLI11_PARSE(app, argc, argv);

    try {
        cltj::compact_ltj_metatrie_hash_rb rb;
        if (!sdsl::load_from_file(rb, rb_file)) {
            std::cerr << "ERROR: cannot load " << rb_file << std::endl;
            return 2;
        }
        const auto& root = rb.root();
        bool ok = true;

        {
            cltj::compact_ltj_metatrie x;
            if (!sdsl::load_from_file(x, x_file)) {
                std::cerr << "ERROR: cannot load " << x_file << std::endl;
                return 2;
            }
            for (int t = 0; t < 6; t++) {
                const auto* rt = rb.get_trie(t);
                const bool full = (t % 2 == 0);
                const uint64_t deg = x.get_trie(t)->root_degree();
                if (!full) {
                    if (rt->dropped_root_labels() != 0) {
                        std::cerr << names[t] << ": root_deleted " << rt->dropped_root_labels() << ", expected 0" << std::endl;
                        ok = false;
                    }
                    continue;
                }
                const auto& xseq = x.get_trie(t)->seq;
                const auto st = static_cast<state_type>(t / 2);
                uint64_t root_bad = 0;
                if (rt->root_degree() != deg || rt->dropped_root_labels() != deg || rt->node_has_hash(0)) {
                    std::cerr << names[t] << ": root degree " << rt->root_degree() << ", root_deleted "
                              << rt->dropped_root_labels() << ", hashed " << rt->node_has_hash(0) << "; X root degree "
                              << deg << std::endl;
                    ok = false;
                }
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
                std::cout << "trie=" << t << " name=" << names[t] << " root_degree=" << deg
                          << " root_mismatches=" << root_bad << std::endl;
                if (root_bad)
                    ok = false;
            }
        }

        cltj::compact_ltj_metatrie_hash h;
        if (!sdsl::load_from_file(h, h_file)) {
            std::cerr << "ERROR: cannot load " << h_file << std::endl;
            return 2;
        }

        for (int full_i : {0, 2, 4}) {
            const int part_i = full_i + 1;
            const auto st = static_cast<state_type>(full_i / 2);
            const auto& rf = *rb.get_trie(full_i);
            const auto& hf = *h.get_trie(full_i);
            const auto& rp = *rb.get_trie(part_i);
            const auto& hp = *h.get_trie(part_i);
            trie_report rep_f, rep_p;
            block_matcher<std::decay_t<decltype(rf)>, std::decay_t<decltype(hf)>> mf{rf, hf, full_i, rep_f};
            block_matcher<std::decay_t<decltype(rp)>, std::decay_t<decltype(hp)>> mp{rp, hp, part_i, rep_p};
            std::vector<std::pair<uint64_t, uint64_t>> level2;

            const uint64_t deg = rf.root_degree();
            if (hf.root_degree() != deg) {
                rep_f.fail(full_i, "root degree differs from H: " + std::to_string(deg) + " vs " +
                                       std::to_string(hf.root_degree()));
            } else {
                uint64_t x = root.next(st, 0);
                for (uint64_t i = 0; i < deg; ++i, x = root.next(st, x + 1)) {
                    const uint64_t ph = root_position(hf, static_cast<uint32_t>(x));
                    if (ph == deg) {
                        rep_f.fail(full_i, "root child " + std::to_string(x) + " is not a child of the root of H");
                        continue;
                    }
                    level2.clear();
                    if (mf.compare(rf.nodeselect(i), hf.nodeselect(ph), &level2))
                        for (const auto& [pr, phh] : level2)
                            mf.compare(rf.nodeselect(pr), hf.nodeselect(phh), nullptr);
                    mp.compare(rp.nodeselect(i - 1), hp.nodeselect(ph - 1), nullptr);
                }
            }

            auto finish = [&](int t, const auto& trie, trie_report& rep, uint64_t root_labels) {
                if (rep.labels + root_labels + 1 != trie.louds_size())
                    rep.fail(t, "matched blocks hold " + std::to_string(rep.labels + root_labels) + " labels, |B|-1=" +
                                    std::to_string(trie.louds_size() - 1));
                std::cout << "trie=" << t << " name=" << names[t] << " blocks=" << rep.blocks
                          << " hashed_blocks=" << rep.hashed_blocks << " labels=" << rep.labels
                          << " mismatches=" << rep.errors << std::endl;
                if (rep.errors)
                    ok = false;
            };
            finish(full_i, rf, rep_f, deg);
            finish(part_i, rp, rep_p, 0);
        }

        std::cout << (ok ? "RESULT: ROOT BITMAP MATCHES X AND LABELS BY PATH MATCH H" : "RESULT: FAIL") << std::endl;
        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    }
}
