#pragma once
#include "sep_bitmap.hpp"
#include "step_table.hpp"
#include <cstdint>
#include <iostream>
#include <sdsl/structure_tree.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cltj {
namespace overlay {

struct CheckReport {
    uint64_t plain_nodes = 0;
    uint64_t hashed_nodes = 0;
    uint64_t plain_labels = 0;
    uint64_t hashed_labels = 0;
    uint64_t comparisons = 0;
    uint64_t errors = 0;
    std::vector<std::string> messages;  // the first few errors

    bool ok() const { return errors == 0; }

    void fail(const std::string& msg) {
        if (messages.size() < 8)
            messages.push_back(msg);
        errors++;
    }
};

inline void print_report(std::ostream& os, const std::string& name, const CheckReport& r) {
    os << "  " << name << ": " << (r.ok() ? "PASS" : "FAIL") << "  plain_nodes=" << r.plain_nodes
       << " hashed_nodes=" << r.hashed_nodes << " plain_labels=" << r.plain_labels
       << " hashed_labels=" << r.hashed_labels << " comparisons=" << r.comparisons << " errors=" << r.errors
       << "\n";
    for (const auto& m : r.messages)
        os << "    " << m << "\n";
}

// The key cursor of the hashed node v emits exactly deg keys, the k-th with slot k and key L[v + k].
template <class Trie>
void check_cursor(const Trie& trie, uint64_t v, uint64_t deg, CheckReport& rep) {
    const auto& seq = trie.seq;
    uint64_t k = 0;
    bool extra = false;
    for (auto cur = trie.hash_keys(v); cur.next(); k++) {
        if (k >= deg) {
            extra = true;
            break;
        }
        rep.comparisons++;
        if (cur.slot() != k || cur.key() != seq[v + k]) {
            std::ostringstream m;
            m << "v=" << v << " k=" << k << ": cursor (slot " << cur.slot() << ", key " << cur.key()
              << ") vs L=" << seq[v + k];
            rep.fail(m.str());
        }
    }
    if (extra || k != deg) {
        std::ostringstream m;
        m << "v=" << v << ": cursor emitted " << (extra ? "more than" : "only") << " " << k
          << " keys, deg=" << deg;
        rep.fail(m.str());
    }
}

/**
 * @brief Ground truth of the dual phase, for any translation policy.
 *
 * Walks the nodes of @p trie in position order, keeping its own count of the labels of
 * non-hashed nodes seen so far (v - deleted_before(v)) and of hashed nodes (r), and checks:
 *   - plain_start(v) equals that count, for every node, and dict_idx(v) == r for hashed ones;
 *   - non-hashed v: plain_start(v) + deg(v) <= |L_plain| and
 *     L_plain[plain_start(v) + k] == L[v + k] for every k < deg(v);
 *   - hashed v: deg(v) >= threshold, dict_idx(v) < mphf_count(), its MPHF has deg(v) keys,
 *     and the key cursor emits exactly deg(v) keys with slot k and key L[v + k];
 *   - the blocks cover L_plain and L (minus its mock) exactly, and r ends at mphf_count().
 */
template <class Trie>
CheckReport check_dual_phase(const Trie& trie) {
    CheckReport rep;
    const auto& seq = trie.seq;
    const auto& plain = trie.seq_plain();
    const auto& ov = trie.overlay();
    const uint64_t louds = trie.louds_size();
    uint64_t plain_cursor = 0;
    uint64_t r = 0;

    for (uint64_t v = 0, deg = 0; v + 1 < louds; v += deg) {
        deg = trie.children(v);
        const uint64_t s = ov.plain_start(v);
        rep.comparisons++;
        if (s != plain_cursor) {
            std::ostringstream m;
            m << "v=" << v << ": plain_start=" << s << ", expected " << plain_cursor;
            rep.fail(m.str());
        }

        if (ov.has_dict(v)) {
            rep.comparisons += 2;
            if (deg < trie.threshold()) {
                std::ostringstream m;
                m << "v=" << v << ": hashed with deg=" << deg << " < threshold=" << trie.threshold();
                rep.fail(m.str());
            }
            if (ov.dict_idx(v) != r) {
                std::ostringstream m;
                m << "v=" << v << ": dict_idx=" << ov.dict_idx(v) << ", expected " << r;
                rep.fail(m.str());
            }
            rep.comparisons++;
            if (ov.dict_idx(v) >= trie.mphf_count()) {
                std::ostringstream m;
                m << "v=" << v << ": dict_idx=" << ov.dict_idx(v) << " >= mphf_count=" << trie.mphf_count();
                rep.fail(m.str());
            } else if (trie.hash_size(v) != deg) {
                std::ostringstream m;
                m << "v=" << v << ": MPHF has " << trie.hash_size(v) << " keys, deg=" << deg;
                rep.fail(m.str());
            } else {
                check_cursor(trie, v, deg, rep);
            }
            r++;
            rep.hashed_nodes++;
            rep.hashed_labels += deg;
        } else {
            rep.comparisons++;
            if (s + deg > plain.size()) {
                std::ostringstream m;
                m << "v=" << v << ": plain_start+deg=" << s + deg << " > |L_plain|=" << plain.size();
                rep.fail(m.str());
            } else {
                for (uint64_t k = 0; k < deg; k++) {
                    rep.comparisons++;
                    if (plain[s + k] != seq[v + k]) {
                        std::ostringstream m;
                        m << "v=" << v << " k=" << k << ": L_plain=" << plain[s + k] << " vs L=" << seq[v + k];
                        rep.fail(m.str());
                    }
                }
            }
            plain_cursor += deg;
            rep.plain_nodes++;
            rep.plain_labels += deg;
        }
    }

    rep.comparisons += 3;
    if (plain_cursor != plain.size()) {
        std::ostringstream m;
        m << "non-hashed blocks hold " << plain_cursor << " labels, |L_plain|=" << plain.size();
        rep.fail(m.str());
    }
    if (rep.plain_labels + rep.hashed_labels + 1 != seq.size()) {
        std::ostringstream m;
        m << "blocks hold " << rep.plain_labels + rep.hashed_labels << " labels, |L|-1=" << seq.size() - 1;
        rep.fail(m.str());
    }
    if (r != trie.mphf_count()) {
        std::ostringstream m;
        m << r << " hashed nodes, " << trie.mphf_count() << " MPHFs";
        rep.fail(m.str());
    }
    return rep;
}

/**
 * @brief Consistency of scheme A's L_sep against B.
 *
 * For the i-th non-hashed node v: select1(L_sep, i + 1) == plain_start(v) and
 * select1(L_sep, i + 2) - select1(L_sep, i + 1) == deg(v), the trailing 1 at |L_plain|
 * closing the last block. L_sep has |L_plain| + 1 bits and one 1 per non-hashed node
 * plus the trailing one.
 */
template <class Trie>
CheckReport check_sep_bitmap(const Trie& trie) {
    CheckReport rep;
    const auto& ov = trie.overlay();
    const uint64_t louds = trie.louds_size();
    const uint64_t ones = ov.lsep_ones();
    uint64_t i = 0;

    rep.comparisons++;
    if (ov.lsep_size() != trie.seq_plain().size() + 1) {
        std::ostringstream m;
        m << "|L_sep|=" << ov.lsep_size() << ", expected |L_plain|+1=" << trie.seq_plain().size() + 1;
        rep.fail(m.str());
    }

    for (uint64_t v = 0, deg = 0; v + 1 < louds; v += deg) {
        deg = trie.children(v);
        if (ov.has_dict(v)) {
            rep.hashed_nodes++;
            continue;
        }
        if (i + 2 > ones) {
            std::ostringstream m;
            m << "v=" << v << ": non-hashed node #" << i << " but L_sep has only " << ones << " ones";
            rep.fail(m.str());
            return rep;
        }
        const uint64_t s = ov.lsep_select(i + 1);
        const uint64_t next = ov.lsep_select(i + 2);
        rep.comparisons += 2;
        if (s != ov.plain_start(v)) {
            std::ostringstream m;
            m << "v=" << v << ": select1(L_sep, i+1)=" << s << " vs plain_start=" << ov.plain_start(v);
            rep.fail(m.str());
        }
        if (next - s != deg) {
            std::ostringstream m;
            m << "v=" << v << ": separator gap " << next - s << " vs deg=" << deg;
            rep.fail(m.str());
        }
        i++;
        rep.plain_nodes++;
        rep.plain_labels += deg;
    }

    rep.comparisons++;
    if (ones != i + 1) {
        std::ostringstream m;
        m << "L_sep has " << ones << " ones, expected " << i + 1;
        rep.fail(m.str());
    }
    return rep;
}

/**
 * @brief Consistency of scheme C's table against B.
 *
 * The r-th hashed node v in position order has hashed_start[r] == v and
 * cum_deleted[r] - cum_deleted[r - 1] == deg(v); the table has one row per MPHF and its
 * last cum_deleted is |L| - 1 - |L_plain|.
 */
template <class Trie>
CheckReport check_step_table(const Trie& trie) {
    CheckReport rep;
    const auto& ov = trie.overlay();
    const uint64_t louds = trie.louds_size();
    const uint64_t rows = ov.hashed_count();
    uint64_t r = 0;
    uint64_t cum = 0;

    rep.comparisons++;
    if (rows != trie.mphf_count()) {
        std::ostringstream m;
        m << rows << " rows, " << trie.mphf_count() << " MPHFs";
        rep.fail(m.str());
    }

    for (uint64_t v = 0, deg = 0; v + 1 < louds; v += deg) {
        deg = trie.children(v);
        if (!ov.has_dict(v)) {
            rep.plain_nodes++;
            rep.plain_labels += deg;
            continue;
        }
        if (r >= rows) {
            std::ostringstream m;
            m << "v=" << v << ": hashed node #" << r << " but the table has only " << rows << " rows";
            rep.fail(m.str());
            return rep;
        }
        cum += deg;
        rep.comparisons += 2;
        if (ov.hashed_start(r) != v) {
            std::ostringstream m;
            m << "r=" << r << ": hashed_start=" << ov.hashed_start(r) << " vs v=" << v;
            rep.fail(m.str());
        }
        if (ov.cum_deleted(r) != cum) {
            std::ostringstream m;
            m << "r=" << r << ": cum_deleted=" << ov.cum_deleted(r) << ", expected " << cum;
            rep.fail(m.str());
        }
        r++;
        rep.hashed_nodes++;
        rep.hashed_labels += deg;
    }

    rep.comparisons += 2;
    if (r != rows) {
        std::ostringstream m;
        m << r << " hashed nodes, " << rows << " rows";
        rep.fail(m.str());
    }
    if (cum + trie.seq_plain().size() + 1 != trie.seq.size()) {
        std::ostringstream m;
        m << "hashed blocks hold " << cum << " labels, |L|-1-|L_plain|=" << trie.seq.size() - 1 - trie.seq_plain().size();
        rep.fail(m.str());
    }
    return rep;
}

//! The consistency check of the policy of @p trie (check_sep_bitmap or check_step_table).
template <class Trie>
CheckReport check_policy(const Trie& trie, const SepBitmapOverlay&) {
    return check_sep_bitmap(trie);
}

template <class Trie>
CheckReport check_policy(const Trie& trie, const StepTableOverlay&) {
    return check_step_table(trie);
}

template <class Trie>
CheckReport check_policy(const Trie& trie) {
    return check_policy(trie, trie.overlay());
}

/**
 * @brief Serialization of @p trie without its format tag and its overlay.
 *
 * The overlay is the only member a policy changes, so two tries built from the same data
 * with different policies have to give the same bytes here.
 */
template <class Trie>
std::string serialized_without_overlay(const Trie& trie) {
    std::stringstream ss;
    sdsl::structure_tree_node root("root", "root");
    trie.serialize(ss, &root, "trie");
    const std::string bytes = ss.str();
    const sdsl::structure_tree_node* node = root.children.begin()->second.get();
    uint64_t before = 0;
    uint64_t overlay = 0;
    for (const auto& c : node->children) {
        const std::string& n = c.second->name;
        if (n == "bv" || n == "seq" || n == "seq_plain" || n == "succ0" || n == "select0" || n == "threshold")
            before += c.second->size;
        else if (n == "overlay")
            overlay = c.second->size;
    }
    const uint64_t tag = sizeof(uint64_t);
    std::stringstream ov;
    trie.overlay().serialize(ov);
    if (bytes.compare(tag + before, overlay, ov.str()) != 0)
        throw std::logic_error("serialized_without_overlay: member sizes do not locate the overlay");
    return bytes.substr(tag, before) + bytes.substr(tag + before + overlay);
}

/**
 * @brief Differential check of two tries built from the same data with different policies.
 *
 * At every block start v and at the sentinel |B| - 1, has_dict, dict_idx and plain_start
 * agree; B, L, L_plain and the MPHFs agree, compared as the serialization without the
 * overlay, and the key cursor of every hashed node emits the same keys.
 */
template <class TrieA, class TrieC>
CheckReport check_overlay_equal(const TrieA& a, const TrieC& c) {
    CheckReport rep;
    const auto& oa = a.overlay();
    const auto& oc = c.overlay();
    const uint64_t louds = a.louds_size();

    rep.comparisons += 2;
    if (louds != c.louds_size() || a.mphf_count() != c.mphf_count()) {
        std::ostringstream m;
        m << "|B| " << louds << " vs " << c.louds_size() << ", MPHFs " << a.mphf_count() << " vs " << c.mphf_count();
        rep.fail(m.str());
        return rep;
    }

    auto compare_at = [&](uint64_t v) {
        rep.comparisons += 3;
        const bool ha = oa.has_dict(v), hc = oc.has_dict(v);
        const uint64_t ra = oa.dict_idx(v), rc = oc.dict_idx(v);
        const uint64_t pa = oa.plain_start(v), pc = oc.plain_start(v);
        if (ha != hc || ra != rc || pa != pc) {
            std::ostringstream m;
            m << "v=" << v << ": has_dict " << ha << "/" << hc << ", dict_idx " << ra << "/" << rc
              << ", plain_start " << pa << "/" << pc;
            rep.fail(m.str());
        }
        return ha && hc;
    };

    for (uint64_t v = 0, deg = 0; v + 1 < louds; v += deg) {
        deg = a.children(v);
        rep.comparisons++;
        if (c.children(v) != deg) {
            std::ostringstream m;
            m << "v=" << v << ": deg " << deg << " vs " << c.children(v);
            rep.fail(m.str());
            return rep;
        }
        if (!compare_at(v)) {
            rep.plain_nodes++;
            rep.plain_labels += deg;
            continue;
        }
        auto ka = a.hash_keys(v);
        auto kc = c.hash_keys(v);
        for (uint64_t k = 0;; k++) {
            const bool more_a = ka.next();
            const bool more_c = kc.next();
            rep.comparisons++;
            if (more_a != more_c) {
                std::ostringstream m;
                m << "v=" << v << ": cursors emit a different number of keys";
                rep.fail(m.str());
                break;
            }
            if (!more_a)
                break;
            if (ka.slot() != kc.slot() || ka.key() != kc.key()) {
                std::ostringstream m;
                m << "v=" << v << " k=" << k << ": cursor (" << ka.slot() << ", " << ka.key() << ") vs ("
                  << kc.slot() << ", " << kc.key() << ")";
                rep.fail(m.str());
                break;
            }
        }
        rep.hashed_nodes++;
        rep.hashed_labels += deg;
    }
    compare_at(louds - 1);

    rep.comparisons++;
    if (serialized_without_overlay(a) != serialized_without_overlay(c))
        rep.fail("serialization without the overlay differs (B, L, L_plain, supports, threshold or MPHFs)");
    return rep;
}

}  // namespace overlay
}  // namespace cltj
