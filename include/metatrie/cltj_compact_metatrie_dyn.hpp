#ifndef CLTJ_COMPACT_METATRIE_DYN_HPP
#define CLTJ_COMPACT_METATRIE_DYN_HPP

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <sdsl/bit_vectors.hpp>
#include <cds/dyn_louds.hpp>
#include <cltj_config.hpp>

namespace cltj {

    template<uint default_width = 32>
    class compact_metatrie_dyn {


    public:

        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        dyn_cds::dyn_louds m_seq;
        size_type m_root_degree;

        void copy(const compact_metatrie_dyn &o) {
            m_seq = o.m_seq;
            m_root_degree = o.m_root_degree;
        }

        /*inline size_type rank0(const size_type i) const {
            return i - m_rank1(i);
        }*/

    public:

        const dyn_cds::dyn_louds &seq = m_seq;

        compact_metatrie_dyn(const uint width = default_width) {
            m_seq = dyn_cds::dyn_louds(width);
        }
 
        compact_metatrie_dyn(sdsl::int_vector<> &s, sdsl::bit_vector &bv,
                         const uint width = default_width) {

            m_seq = dyn_cds::dyn_louds(bv.data(), s.data(), bv.size(), width);
            m_root_degree = m_seq.next(1);
        }

        //! Copy constructor
        compact_metatrie_dyn(const compact_metatrie_dyn &o) {
            copy(o);
        }

        //! Move constructor
        compact_metatrie_dyn(compact_metatrie_dyn &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        compact_metatrie_dyn &operator=(const compact_metatrie_dyn &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        compact_metatrie_dyn &operator=(compact_metatrie_dyn &&o) {
            if (this != &o) {
                m_seq = std::move(o.m_seq);
                m_root_degree = o.m_root_degree;
            }
            return *this;
        }

        void swap(compact_metatrie_dyn &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_seq, o.m_seq);
            std::swap(m_root_degree, o.m_root_degree);
        }


        /*
            Degree of the trie root
        */

         size_type root_degree() {
             return m_root_degree;
         }

         void inc_root_degree() {
            ++m_root_degree;
        }

        void dec_root_degree() {
            --m_root_degree;
        }

        void insert(size_type node_pos, value_type v_seq, bool v_bv, bool first) {
            m_seq.insert(node_pos, v_bv, v_seq, first);
        }


        void remove(size_type node_pos, bool more) {
            m_seq.remove(node_pos, more);
        }

        inline size_type child(uint32_t it, uint32_t n, uint32_t gap=1) const {
            return m_seq.select(it + gap + n);
        }

        /*
            Receives index of node whos children we want to count
            Returns how many children said node has
        */
        size_type children(size_type i) const {
            //return m_louds.select(m_louds.rank(i+1)+1) - i;
            return m_seq.next(i+1) - i;
            //return m_succ(i + 1) - i;
        }

        size_type first_child(size_type i) const {
            return i;
        }


        inline size_type nodeselect(size_type i) const {
            return m_seq.select(i + 2);
        }

        std::pair<value_type, size_type> next(size_type i, size_type j, value_type val) {
            return m_seq.next(i, j, val);
        }

        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
            return m_seq.next(i, f, val);
        }

        pair<uint32_t, uint32_t> binary_search(uint32_t val, uint32_t i, uint32_t f) const {
            if (m_seq[f] < val) return make_pair(0, f + 1);
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

        bool check() {
            return m_seq.check();
        }

        void split() {
            m_seq.split();
        }


        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_seq.serialize(out, child, "seq");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_seq.load(in);
            m_root_degree = m_seq.next(1);
        }

    };
}
#endif
