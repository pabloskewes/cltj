// Lookups inside one trie node, with the structures that store the children
// of a node in each system: the MPHF of H (GlGh or PackedGlGh, with
// quotients), the Elias-Fano lists of CLTJ-EF and the fixed-width labels of X
// searched with galloping.
//
// Every structure answers two operations over the sorted labels of one node:
//   lookup(c)          membership of c, as in a first visit to the node.
//   seek(c, a, cur)    first label >= c from position a (cursor cur), as in a
//                      leap that continues where the previous one ended.
// Both return a 64-bit word that depends on the answer, so that a benchmark
// can chain the next query to it.

#pragma once

#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/glgh.hpp>
#include <hashing/storage/packed_glgh.hpp>

#include <sdsl/int_vector.hpp>
#include <sdsl/sd_vector.hpp>
#include <sdsl/util.hpp>

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cltj {
namespace node_lookup {

static constexpr uint64_t no_cursor = ~0ULL;

// Answer of a lookup or a seek: found flag, position in the node and cursor.
// For the MPHF the position is the slot. For seek, pos == n means "no label >= c".
struct Answer {
    bool found;
    uint64_t pos;
    uint64_t value;
    uint64_t cur;
};

template <typename Storage>
class HashNode {
  public:
    using storage_type = Storage;
    using mphf_type = hashing::MPHF<Storage, hashing::policies::QuotientKey>;

    HashNode() = default;

    explicit HashNode(const std::vector<uint32_t>& keys) {
        if (!m_mphf.build(keys))
            throw std::runtime_error("HashNode: MPHF build failed");
        m_n = keys.size();
    }

    // Copy through serialization, so that copies live at different addresses.
    HashNode clone() const {
        std::stringstream ss;
        m_mphf.serialize(ss);
        HashNode h;
        h.m_mphf.load(ss);
        h.m_n = m_n;
        return h;
    }

    inline Answer lookup(uint32_t c) const {
        auto r = m_mphf.locate(c);
        return {r.first, r.second, c, no_cursor};
    }

    uint64_t size() const { return m_n; }
    uint64_t size_in_bytes() const { return m_mphf.size_in_bytes(); }
    hashing::SizeBreakdown breakdown() const { return m_mphf.get_size_breakdown(); }
    const mphf_type& mphf() const { return m_mphf; }

  private:
    mphf_type m_mphf;
    uint64_t m_n = 0;
};

using GlGhNode = HashNode<hashing::GlGhStorage>;
using PackedGlGhNode = HashNode<hashing::PackedGlGhStorage>;

// R copies of the children of one long node of CLTJ-EF, laid out as in
// compact_trie_ef_t of upstream adriangbrandon/cltj, branch cltj-ef, commit
// 2424f07 (include/trie/cltj_compact_trie_ef.hpp): the lists are concatenated
// in one sdsl::sd_vector (one density class, list k stored as base_k + x -
// first) and each list has a 64-byte header. ef_value, ef_successor and
// seek_long are that code; positions a are local to the list. The universe of
// a list is the range of the node.
class EFNodes {
  public:
    typedef sdsl::sd_vector<> ef_type;
    static const uint64_t scan_elems = 8;

    EFNodes() = default;

    EFNodes(const std::vector<uint32_t>& keys, uint64_t copies) {
        if (keys.empty() || copies == 0)
            throw std::runtime_error("EFNodes: empty node");
        m_n = keys.size();
        m_copies = copies;
        const uint64_t first = keys.front(), last = keys.back(), u = last - first + 1;
        sdsl::sd_vector_builder b(u * copies, m_n * copies);
        for (uint64_t k = 0; k < copies; ++k)
            for (uint32_t x : keys)
                b.set(k * u + (x - first));
        m_sd = ef_type(b);
        m_hdr = sdsl::int_vector<64>(copies * H_WORDS, 0);
        for (uint64_t k = 0; k < copies; ++k) {
            uint64_t* h = m_hdr.data() + k * H_WORDS;
            h[H_SC] = k * m_n;
            h[H_LEN] = m_n;
            h[H_BASE] = k * u;
            h[H_FIRST] = first;
            h[H_LAST] = last;
            h[H_HP0] = m_sd.high_1_select(k * m_n + 1);
            if (h_ones(h, m_sd) != k * m_n)
                throw std::runtime_error("EFNodes: inconsistent header");
        }
    }

    inline Answer lookup(uint64_t k, uint32_t c) const {
        Answer a = seek(k, c, 0, no_cursor);
        a.found = a.pos < m_n && a.value == c;
        return a;
    }

    // seek_long of upstream, with i = st + a and f = st + n - 1.
    inline Answer seek(uint64_t k, uint32_t val, uint64_t a, uint64_t cur) const {
        const uint64_t* h = hdr(k);
        if (val > h[H_LAST])
            return {false, m_n, 0, cur};
        const ef_type& sd = m_sd;
        const uint64_t ob = h_ones(h, sd), base = h[H_BASE], fst = h[H_FIRST];
        const uint64_t b = h[H_LEN] - 1;
        uint64_t r = ob + a + 1;
        uint64_t hp = h_pos(h, sd, a, r, cur);
        uint64_t x = ef_value(sd, r, hp) - base + fst;
        if (x < val) {
            bool found = false;
            for (uint64_t g = 0; g < scan_elems && a < b; ++g) {
                ++a;
                ++r;
                hp = sdsl::bits::next(sd.high.data(), hp + 1);
                x = ef_value(sd, r, hp) - base + fst;
                if (x >= val) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                // val <= last, so the successor exists
                ef_successor(sd, base + (val - fst), r, hp);
                a = r - 1 - ob;
                x = ef_value(sd, r, hp) - base + fst;
            }
        }
        return {x == val, a, x, hp};
    }

    uint64_t size() const { return m_n; }
    uint64_t copies() const { return m_copies; }
    uint64_t size_in_bytes() const { return sdsl::size_in_bytes(m_sd) + sdsl::size_in_bytes(m_hdr); }
    uint64_t sd_bytes() const { return sdsl::size_in_bytes(m_sd); }
    uint64_t low_width() const { return m_sd.wl; }

  private:
    enum { H_SC = 0, H_LEN, H_BASE, H_FIRST, H_LAST, H_HP0, H_R1, H_R2, H_WORDS };
    ef_type m_sd;
    sdsl::int_vector<64> m_hdr;
    uint64_t m_n = 0, m_copies = 0;

    inline const uint64_t* hdr(uint64_t k) const { return m_hdr.data() + k * H_WORDS; }
    static inline uint64_t h_ones(const uint64_t* h, const ef_type& sd) { return h[H_HP0] - (h[H_BASE] >> sd.wl); }

    static inline uint64_t h_pos(const uint64_t* h, const ef_type& sd, uint64_t a, uint64_t r, uint64_t cur) {
        if (cur != no_cursor)
            return cur;
        if (a == 0)
            return h[H_HP0];
        return sd.high_1_select(r);
    }

    //! value of the r-th one (1-based) of an sd_vector, given its position hp in H
    static inline uint64_t ef_value(const ef_type& sd, uint64_t r, uint64_t hp) {
        return ((hp - r + 1) << sd.wl) | sd.low[r - 1];
    }

    //! first element of sd that is >= target: returns its rank r (1-based) and position hp in H.
    //! Requires that such element exists. One select0 plus a forward scan of H.
    static inline void ef_successor(const ef_type& sd, uint64_t target, uint64_t& r, uint64_t& hp) {
        const uint64_t high_val = target >> sd.wl;
        const uint64_t val_low = target & sdsl::bits::lo_set[sd.wl];
        hp = high_val ? sd.high_0_select(high_val) + 1 : 0;
        r = hp - high_val + 1;
        const uint64_t* data = sd.high.data();
        while (true) {
            if ((data[hp >> 6] >> (hp & 63)) & 1ULL) {
                if (sd.low[r - 1] >= val_low)
                    return;
                ++hp;
                ++r;
            } else {
                hp = sdsl::bits::next(data, hp + 1);
                return;
            }
        }
    }
};

// R copies of the labels of one node in X: one plain array with the width of
// the level, node k at positions [k n, (k + 1) n), searched with the galloping
// of upstream adriangbrandon/cltj, branch master
// (include/trie/cltj_compact_trie.hpp, compact_trie::gallop), copied as is.
class GallopNodes {
  public:
    GallopNodes() = default;

    GallopNodes(const std::vector<uint32_t>& keys, uint8_t width, uint64_t copies) {
        if (keys.empty() || copies == 0)
            throw std::runtime_error("GallopNodes: empty node");
        if (width < 64 && (uint64_t(keys.back()) >> width) != 0)
            throw std::runtime_error("GallopNodes: label wider than the level width");
        m_n = keys.size();
        m_copies = copies;
        m_seq = sdsl::int_vector<>(m_n * copies, 0, width);
        for (uint64_t k = 0; k < copies; ++k)
            for (uint64_t i = 0; i < m_n; ++i)
                m_seq[k * m_n + i] = keys[i];
    }

    inline Answer lookup(uint64_t k, uint32_t c) const {
        Answer a = seek(k, c, 0, no_cursor);
        a.found = a.pos < m_n && a.value == c;
        return a;
    }

    inline Answer seek(uint64_t k, uint32_t val, uint64_t a, uint64_t) const {
        const uint64_t st = k * m_n;
        auto r = gallop(m_seq, 0, val, st + a, st + m_n - 1);
        uint64_t pos = r.second - st;
        return {pos < m_n && r.first == val, pos, r.first, no_cursor};
    }

    uint64_t size() const { return m_n; }
    uint64_t copies() const { return m_copies; }
    uint64_t size_in_bytes() const { return sdsl::size_in_bytes(m_seq); }
    uint64_t width() const { return m_seq.width(); }

  private:
    sdsl::int_vector<> m_seq;
    uint64_t m_n = 0, m_copies = 0;

    //! Galloping search for the first position k in [i, f] with seq[k - off] >= val.
    //! Probes i, i+1, i+3, i+7, ... and finishes with binary search.
    template <class Seq>
    static std::pair<uint32_t, uint64_t> gallop(const Seq& seq, uint64_t off, uint32_t val, uint64_t i, uint64_t f) {
        uint64_t lo = i, step = 1, hi = i;
        if (seq[i - off] >= val)
            return std::make_pair((uint32_t)seq[i - off], i);
        while (true) {
            hi = lo + step;
            if (hi >= f) {
                hi = f;
                break;
            }
            if (seq[hi - off] >= val)
                break;
            lo = hi;
            step <<= 1;
        }
        // seq[lo] < val; the answer is in (lo, hi] if seq[hi] >= val
        if (seq[hi - off] < val)
            return std::make_pair(0, f + 1);
        ++lo;
        while (lo < hi) {
            uint64_t mid = (lo + hi) / 2;
            if (seq[mid - off] < val)
                lo = mid + 1;
            else
                hi = mid;
        }
        return std::make_pair((uint32_t)seq[lo - off], lo);
    }
};

// R copies of the MPHF of one node, one object per copy as in the m_mphfs of
// compact_metatrie_hash.
template <typename Storage>
class HashNodes {
  public:
    HashNodes(HashNode<Storage>&& base, uint64_t copies) {
        m_v.reserve(copies);
        m_v.push_back(std::move(base));
        for (uint64_t i = 1; i < copies; ++i)
            m_v.push_back(m_v[0].clone());
    }

    inline Answer lookup(uint64_t k, uint32_t c) const { return m_v[k].lookup(c); }

    uint64_t size() const { return m_v[0].size(); }
    uint64_t copies() const { return m_v.size(); }
    uint64_t size_in_bytes() const { return m_v[0].size_in_bytes(); }

  private:
    std::vector<HashNode<Storage>> m_v;
};

}  // namespace node_lookup
}  // namespace cltj
