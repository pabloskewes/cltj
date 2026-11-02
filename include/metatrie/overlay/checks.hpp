#pragma once
#include <cstdint>
#include <iostream>
#include <sstream>
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

/**
 * @brief Ground truth of the dual phase, for any translation policy.
 *
 * Walks the nodes of @p trie in position order, keeping its own count of the labels of
 * non-hashed nodes seen so far (v - deleted_before(v)) and of hashed nodes (r), and checks:
 *   - plain_start(v) equals that count, for every node, and dict_idx(v) == r for hashed ones;
 *   - non-hashed v: plain_start(v) + deg(v) <= |L_plain| and
 *     L_plain[plain_start(v) + k] == L[v + k] for every k < deg(v);
 *   - hashed v: deg(v) >= threshold, and the key cursor emits exactly deg(v) keys with
 *     slot k and key L[v + k];
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

}  // namespace overlay
}  // namespace cltj
