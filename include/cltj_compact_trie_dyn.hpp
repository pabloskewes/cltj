#ifndef CLTJ_COMPACT_TRIE_DYN_HPP
#define CLTJ_COMPACT_TRIE_DYN_HPP

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <sdsl/bit_vectors.hpp>
#include <dyn_bit_vector.hpp>
#include <dyn_array.hpp>
#include <cltj_config.hpp>

namespace cltj {

    template<uint default_width = 32>
    class compact_trie_dyn {


    public:

        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        dyn_cds::dyn_array m_seq;
        dyn_cds::dyn_bit_vector m_dyn_bv;

        void copy(const compact_trie_dyn &o) {
            m_dyn_bv = o.m_dyn_bv;
            m_seq = o.m_seq;
        }

        /*inline size_type rank0(const size_type i) const {
            return i - m_rank1(i);
        }*/

    public:

        const dyn_cds::dyn_array &seq = m_seq;


        compact_trie_dyn(const uint width = default_width) {
            m_seq = dyn_cds::dyn_array(width);
        }

        compact_trie_dyn(const spo_triple &triple, const spo_order_type &order,
                         const bool skip_level, const uint width = default_width) {
            sdsl::int_vector<> s(2+!skip_level);
            for(uint64_t l = 0; l < s.size(); ++l) {
                s[l] = triple[order[l+skip_level]];
            }
            auto bv = sdsl::bit_vector(2+!skip_level+1, 1);
            m_dyn_bv = dyn_cds::dyn_bit_vector(bv.data(), bv.size());
            m_seq = dyn_cds::dyn_array(s.data(), s.size(), width);
        }

        compact_trie_dyn(const std::vector<uint32_t> &syms, const std::vector<size_type> &lengths,
                         const uint width = default_width) {
            auto bv = sdsl::bit_vector(syms.size()+1, 0);
            sdsl::int_vector<> s(syms.size());
            bv[0] = 1;
            uint64_t pos_bv = 0, pos_seq = 0;
            for(const auto &sym : syms) {
                s[pos_seq] = sym;
                ++pos_seq;
            }
            for(const auto &len : lengths) {
                pos_bv = pos_bv + len;
                bv[pos_bv] = 1;
            }

            std::cout << "n_nodes=" << syms.size() << " pos_seq=" << pos_seq << " pos_bv=" << pos_bv << std::endl;
            m_dyn_bv = dyn_cds::dyn_bit_vector(bv.data(), bv.size());
            m_seq = dyn_cds::dyn_array(s.data(), s.size(), s.width());
            //sdsl::util::bit_compress(m_seq);

        }

        //! Copy constructor
        compact_trie_dyn(const compact_trie_dyn &o) {
            copy(o);
        }

        //! Move constructor
        compact_trie_dyn(compact_trie_dyn &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        compact_trie_dyn &operator=(const compact_trie_dyn &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        compact_trie_dyn &operator=(compact_trie_dyn &&o) {
            if (this != &o) {
                m_dyn_bv = std::move(o.m_dyn_bv);
                m_seq = std::move(o.m_seq);
            }
            return *this;
        }

        void swap(compact_trie_dyn &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_dyn_bv, o.m_dyn_bv);
            std::swap(m_seq, o.m_seq);
        }

        void insert(size_type node_pos, value_type v_seq, bool v_bv, bool f_child) {
            m_dyn_bv.insert(node_pos + f_child, v_bv); //+new_node to avoid problems with 0s before 1s
            m_seq.insert(node_pos, v_seq);
        }


        void remove(size_type node_pos, bool first_child) {
            m_dyn_bv.remove(node_pos + first_child); //+first_child to prevent removing the 1 instead of a 0
            m_seq.remove(node_pos);
        }


        /*
            Receives index in bit vector
            Returns index of next 0
        */
        /*inline uint32_t succ0(uint32_t it) const{
            return m_select0(rank0(it) + 1);
        }*/

        /*
            Receives index of current node and the child that is required
            Returns index of the nth child of current node
        */
        inline size_type child(uint32_t it, uint32_t gap, uint32_t n) const {
            return m_dyn_bv.select(it + gap + n);
        }

        /*
            Receives index of node whos children we want to count
            Returns how many children said node has
        */
        size_type children(size_type i) const {
            return m_dyn_bv.select(m_dyn_bv.rank(i+1)+1) - i;
            //return m_succ(i + 1) - i;
        }

        size_type first_child(size_type i) const {
            return i;
        }

        //TODO: update this pair into the rest of classes
        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
            if (m_seq[f] < val) return std::make_pair(0, f + 1);
            uint64_t mid;
            while (i < f) {
                mid = (i + f) / 2;
                if (m_seq[mid] < val) {
                    i = mid + 1;
                } else {
                    f = mid;
                }
            }
            return std::make_pair(m_seq[i], i);
        }

        //return position, equal; where equal means that position contains val
        std::pair<uint64_t, bool> binary_search(uint32_t val, uint32_t i, uint32_t f) {
            if(m_seq[f] < val) return {f+1, false};
            uint32_t mid;
            while(i < f) {
                mid = (i+f) / 2;
                if(m_seq[mid] == val) return {mid, true};
                if(m_seq[mid] < val) {
                    i = mid + 1;
                }else {
                    f = mid;
                }
            }
            return {i, m_seq[i] == val};

        }


        void print() const {
            for (auto i = 0; i < m_dyn_bv.size(); ++i) {
                std::cout << (uint) m_dyn_bv[i] << ", ";
            }
            std::cout << std::endl;

            for(auto i = 0; i < m_seq.size(); ++i) {
                std::cout << (uint) m_seq[i] << ", ";
            }
            std::cout << std::endl;
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_dyn_bv.serialize(out, child, "bv");
            written_bytes += m_seq.serialize(out, child, "seq");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_dyn_bv.load(in);
            m_seq.load(in);
        }

    };
}
#endif