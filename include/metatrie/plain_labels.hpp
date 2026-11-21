#ifndef CLTJ_PLAIN_LABELS_HPP
#define CLTJ_PLAIN_LABELS_HPP

#include <metatrie/overlay/strategy.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace cltj {

// Storage of L_plain for compact_metatrie_hash. seek(val, i, f, off) searches (a suffix of) the block
// of one node, whose label at LOUDS position k is L_plain[k - off]: it returns the first k in [i, f]
// with L_plain[k - off] >= val and its label, or {0, f + 1}.

//! L_plain in one int_vector with a single width; binary search.
class single_width_labels {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;

    static constexpr bool level_widths = false;

  private:
    sdsl::int_vector<> m_seq;

  public:
    single_width_labels() = default;

    //! @p n2 is not used: every label keeps the width of the widest one.
    single_width_labels(sdsl::int_vector<>&& plain, size_type /*n2*/) : m_seq(std::move(plain)) {
        sdsl::util::bit_compress(m_seq);
    }

    size_type size() const { return m_seq.size(); }

    value_type operator[](size_type i) const { return m_seq[i]; }

    value_type access(size_type i) const { return m_seq[i]; }

    std::pair<uint32_t, uint32_t> seek(uint32_t val, uint32_t i, uint32_t f, size_type off) const {
        if (m_seq[f - off] < val)
            return std::make_pair(0, f + 1);
        uint32_t mid;
        while (i < f) {
            mid = (i + f) / 2;
            if (m_seq[mid - off] < val) {
                i = mid + 1;
            } else {
                f = mid;
            }
        }
        return std::make_pair(value_type(m_seq[i - off]), i);
    }

    uint8_t width() const { return m_seq.width(); }

    bool operator==(const single_width_labels& o) const { return m_seq == o.m_seq; }

    void swap(single_width_labels& o) { std::swap(m_seq, o.m_seq); }

    //! Same bytes as the int_vector alone, so a single-width trie keeps its format.
    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, const std::string& name = "")
        const {
        return m_seq.serialize(out, v, name);
    }

    void load(std::istream& in) { m_seq.load(in); }

    overlay::OverlaySizeBreakdown get_size_breakdown() const {
        overlay::OverlaySizeBreakdown b;
        b.components = {
            {"seq_plain", sdsl::size_in_bytes(m_seq)}
        };
        return b;
    }
};

/**
 * @brief L_plain split by level, each part with its own width; galloping search.
 *
 * L_plain is [level 2 | level 3]: positions [0, n2) are labels of level 2 and [n2, |L_plain|)
 * of level 3. A block never crosses n2, so the level of a search is the level of f - off.
 */
class level_widths_labels {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;

    static constexpr bool level_widths = true;

  private:
    sdsl::int_vector<> m_plain2;
    sdsl::int_vector<> m_plain3;
    size_type m_n2 = 0;

    //! First k in [i, f] with seq[k - off] >= val: probes i, i+1, i+3, i+7, ... then binary search.
    static std::pair<value_type, size_type> gallop(const sdsl::int_vector<>& seq, size_type off, value_type val,
                                                   size_type i, size_type f) {
        size_type lo = i, step = 1, hi = i;
        if (seq[i - off] >= val)
            return {seq[i - off], i};
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
        // seq[lo - off] < val; the answer is in (lo, hi] if seq[hi - off] >= val
        if (seq[hi - off] < val)
            return {0, f + 1};
        ++lo;
        while (lo < hi) {
            const size_type mid = (lo + hi) / 2;
            if (seq[mid - off] < val)
                lo = mid + 1;
            else
                hi = mid;
        }
        return {seq[lo - off], lo};
    }

  public:
    level_widths_labels() = default;

    //! Takes the first @p n2 labels of @p plain as level 2 and the rest as level 3.
    level_widths_labels(sdsl::int_vector<>&& plain, size_type n2) : m_n2(n2) {
        const size_type n = plain.size();
        m_plain2 = sdsl::int_vector<>(n2, 0, plain.width());
        for (size_type k = 0; k < n2; k++)
            m_plain2[k] = plain[k];
        sdsl::util::bit_compress(m_plain2);
        // Level 3 is shifted down in place instead of copied, to keep a single copy of it.
        for (size_type k = n2; k < n; k++)
            plain[k - n2] = plain[k];
        plain.resize(n - n2);
        m_plain3 = std::move(plain);
        sdsl::util::bit_compress(m_plain3);
    }

    size_type size() const { return m_n2 + m_plain3.size(); }

    size_type n2() const { return m_n2; }

    const sdsl::int_vector<>& plain2() const { return m_plain2; }

    const sdsl::int_vector<>& plain3() const { return m_plain3; }

    value_type access(size_type i) const { return i < m_n2 ? m_plain2[i] : m_plain3[i - m_n2]; }

    value_type operator[](size_type i) const { return access(i); }

    std::pair<uint32_t, uint32_t> seek(uint32_t val, uint32_t i, uint32_t f, size_type off) const {
        if (f - off < m_n2)
            return gallop(m_plain2, off, val, i, f);
        return gallop(m_plain3, off + m_n2, val, i, f);
    }

    bool operator==(const level_widths_labels& o) const {
        return m_n2 == o.m_n2 && m_plain2 == o.m_plain2 && m_plain3 == o.m_plain3;
    }

    void swap(level_widths_labels& o) {
        std::swap(m_plain2, o.m_plain2);
        std::swap(m_plain3, o.m_plain3);
        std::swap(m_n2, o.m_n2);
    }

    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, const std::string& name = "")
        const {
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_plain2.serialize(out, child, "plain2");
        written_bytes += m_plain3.serialize(out, child, "plain3");
        written_bytes += sdsl::write_member(m_n2, out, child, "n2");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        m_plain2.load(in);
        m_plain3.load(in);
        sdsl::read_member(m_n2, in);
    }

    overlay::OverlaySizeBreakdown get_size_breakdown() const {
        overlay::OverlaySizeBreakdown b;
        b.components = {
            {"plain2", sdsl::size_in_bytes(m_plain2)},
            {"plain3", sdsl::size_in_bytes(m_plain3)},
            {"n2",     sizeof(m_n2)                 },
        };
        return b;
    }
};

}  // namespace cltj

#endif
