#ifndef CLTJ_COMPACT_METATRIE_HASH_H
#define CLTJ_COMPACT_METATRIE_HASH_H

#include <cds/succ_support_v.hpp>
#include <cltj_config.hpp>
#include <fstream>
#include <iostream>
#include <queue>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/vectors.hpp>
#include <string>
#include <vector>
#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/glgh.hpp>

namespace cltj {

class compact_metatrie_hash {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;

  private:
    using mphf_type = hashing::MPHF<hashing::GlGhStorage, hashing::policies::QuotientKey>;

    sdsl::bit_vector m_bv;
    sdsl::int_vector<> m_seq;
    // sdsl::rank_support_v<1> m_rank1;
    cds::succ_support_v<0> m_succ0;
    sdsl::select_support_mcl<0> m_select0;

    size_type m_root_degree;

    // Hash overlay
    // TODO: Empty until Batch 2 builds MPHFs
    std::vector<mphf_type> m_mphfs;
    sdsl::bit_vector m_has_hash;
    sdsl::rank_support_v<1> m_hash_rank;
    uint32_t m_threshold = 0;

    // Copy is disabled: std::vector<mphf_type> is not copyable (MPHF copy is deleted by design).
    // Might change in the future if we find a good reason to copy it.

    /*inline size_type rank0(const size_type i) const {
      return i - m_rank1(i);
  }*/

  public:
    const sdsl::int_vector<>& seq = m_seq;

    compact_metatrie_hash() = default;

    compact_metatrie_hash(sdsl::bit_vector& _bv, sdsl::int_vector<>& _seq) {
        m_bv = _bv;
        m_seq = _seq;
        sdsl::util::bit_compress(m_seq);
        sdsl::util::init_support(m_succ0, &m_bv);
        sdsl::util::init_support(m_select0, &m_bv);
        m_root_degree = m_succ0(1);
    }

    compact_metatrie_hash(const compact_metatrie_hash&) = delete;
    compact_metatrie_hash& operator=(const compact_metatrie_hash&) = delete;

    //! Move constructor
    compact_metatrie_hash(compact_metatrie_hash&& o) { *this = std::move(o); }

    //! Move Operator=
    compact_metatrie_hash& operator=(compact_metatrie_hash&& o) {
        if (this != &o) {
            m_bv = std::move(o.m_bv);
            m_seq = std::move(o.m_seq);
            // m_rank1 = std::move(o.m_rank1);
            m_succ0 = std::move(o.m_succ0);
            // m_rank1.set_vector(&m_bv);
            m_succ0.set_vector(&m_bv);
            m_select0 = std::move(o.m_select0);
            m_select0.set_vector(&m_bv);
            m_root_degree = o.m_root_degree;
            m_mphfs = std::move(o.m_mphfs);
            m_has_hash = std::move(o.m_has_hash);
            m_hash_rank = std::move(o.m_hash_rank);
            m_hash_rank.set_vector(&m_has_hash);
            m_threshold = o.m_threshold;
        }
        return *this;
    }

    void swap(compact_metatrie_hash& o) {
        // m_bp.swap(bp_support.m_bp); use set_vector to set the supported
        // bit_vector
        std::swap(m_bv, o.m_bv);
        std::swap(m_seq, o.m_seq);
        sdsl::util::swap_support(m_succ0, o.m_succ0, &m_bv, &o.m_bv);
        sdsl::util::swap_support(m_select0, o.m_select0, &m_bv, &o.m_bv);
        std::swap(m_root_degree, o.m_root_degree);
    }

    /*
      Degree of the trie root
  */

    size_type root_degree() const { return m_root_degree; }

    /*
      Receives index of current node and the child that is required
      Returns index of the nth child of current node
  */
    inline size_type child(uint32_t it, uint32_t n) const { return m_select0(it + 1 + n); }

    /*
      Receives index of node whos children we want to count
      Returns how many children said node has
  */
    size_type children(size_type i) const { return m_succ0(i + 1) - i; }

    size_type first_child(size_type i) const { return i; }

    inline size_type nodeselect(size_type i) const { return m_select0(i + 2); }

    pair<uint32_t, uint32_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
        if (m_seq[f] < val)
            return make_pair(0, f + 1);
        uint32_t mid;
        while (i < f) {
            mid = (i + f) / 2;
            if (m_seq[mid] < val) {
                i = mid + 1;
            } else {
                f = mid;
            }
        }
        return make_pair(m_seq[i], i);
    }

    void print() const {
        for (size_type i = 0; i < m_bv.size(); ++i) {
            std::cout << (uint)m_bv[i];
        }
        std::cout << std::endl;
    }

    //! Serializes the data structure into the given ostream
    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "")
        const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_bv.serialize(out, child, "bv");
        written_bytes += m_seq.serialize(out, child, "seq");
        written_bytes += m_succ0.serialize(out, child, "succ0");
        written_bytes += m_select0.serialize(out, child, "select0");
        written_bytes += sdsl::write_member(m_threshold, out, child, "threshold");
        written_bytes += m_has_hash.serialize(out, child, "has_hash");
        written_bytes += m_hash_rank.serialize(out, child, "hash_rank");
        uint32_t n = m_mphfs.size();
        written_bytes += sdsl::write_member(n, out, child, "n_mphfs");
        for (auto& mphf : m_mphfs)
            written_bytes += mphf.serialize(out, child, "mphf");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        m_bv.load(in);
        m_seq.load(in);
        m_succ0.load(in, &m_bv);
        m_select0.load(in, &m_bv);
        m_root_degree = m_succ0(1);
        sdsl::read_member(m_threshold, in);
        m_has_hash.load(in);
        m_hash_rank.load(in, &m_has_hash);
        uint32_t n;
        sdsl::read_member(n, in);
        m_mphfs.resize(n);
        for (auto& mphf : m_mphfs)
            mphf.load(in);
    }
};
}  // namespace cltj
#endif
