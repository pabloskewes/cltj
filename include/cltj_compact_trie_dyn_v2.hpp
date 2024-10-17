#ifndef CLTJ_COMPACT_TRIE_DYN_V2_HPP
#define CLTJ_COMPACT_TRIE_DYN_V2_HPP

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <sdsl/bit_vectors.hpp>
#include <dyn_louds.hpp>
#include <cltj_config.hpp>

namespace cltj {

    template<uint default_width = 32>
    class compact_trie_dyn_v2 {


    public:

        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        dyn_cds::dyn_louds m_louds;

        void copy(const compact_trie_dyn_v2 &o) {
            m_louds = o.m_louds;
        }

        /*inline size_type rank0(const size_type i) const {
            return i - m_rank1(i);
        }*/

    public:

        const dyn_cds::dyn_louds &louds = m_louds;

        compact_trie_dyn_v2(const uint width = default_width) {
            m_louds = dyn_cds::dyn_louds(width);
        }

        compact_trie_dyn_v2(const spo_triple &triple, const spo_order_type &order,
                         const bool skip_level, const uint width = default_width) {
            sdsl::int_vector<> s(3+!skip_level);
            uint64_t l = 0;
            while( l < s.size()-1) {
                s[l] = triple[order[l+skip_level]];
                ++l;
            }
            s[l] = 0; //mock
            auto bv = sdsl::bit_vector(3+!skip_level, 1);
            m_louds = dyn_cds::dyn_louds(bv.data(), s.data(), bv.size(), width);
        }

        compact_trie_dyn_v2(const std::vector<uint32_t> &syms, const std::vector<size_type> &lengths,
                         const uint width = default_width) {
            auto bv = sdsl::bit_vector(syms.size()+1, 0);
            sdsl::int_vector<> s(syms.size()+1);
            bv[0] = 1;
            uint64_t pos_bv = 0, pos_seq = 0;
            for(const auto &sym : syms) {
                s[pos_seq] = sym;
                ++pos_seq;
            }
            s[pos_seq] = 0; //mock
            for(const auto &len : lengths) {
                pos_bv = pos_bv + len;
                bv[pos_bv] = 1;
            }

            std::cout << "n_nodes=" << syms.size() << " pos_seq=" << pos_seq << " pos_bv=" << pos_bv << std::endl;
            m_louds = dyn_cds::dyn_louds(bv.data(), s.data(), bv.size(), width);
            //sdsl::util::bit_compress(m_seq);

        }

        //! Copy constructor
        compact_trie_dyn_v2(const compact_trie_dyn_v2 &o) {
            copy(o);
        }

        //! Move constructor
        compact_trie_dyn_v2(compact_trie_dyn_v2 &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        compact_trie_dyn_v2 &operator=(const compact_trie_dyn_v2 &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        compact_trie_dyn_v2 &operator=(compact_trie_dyn_v2 &&o) {
            if (this != &o) {
                m_louds = std::move(o.m_louds);
            }
            return *this;
        }

        void swap(compact_trie_dyn_v2 &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_louds, o.m_louds);
        }

        void insert(size_type node_pos, value_type v_seq, bool v_bv, bool first) {
            m_louds.insert(node_pos, v_bv, v_seq, first);
        }


        void remove(size_type node_pos, bool more) {
            m_louds.remove(node_pos, more);
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
        inline size_type child(uint32_t it, uint32_t n, uint32_t gap=1) const {
            return m_louds.select(it + gap + n);
        }

        /*
            Receives index of node whos children we want to count
            Returns how many children said node has
        */
        size_type children(size_type i) const {
            //return m_louds.select(m_louds.rank(i+1)+1) - i;
            return m_louds.next(i+1) - i;
            //return m_succ(i + 1) - i;
        }

        size_type first_child(size_type i) const {
            return i;
        }

        bool check() {
            return m_louds.check();
        }

        void check_print() {
            m_louds.check_print();
        }

        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
            if (m_louds[f] < val) return std::make_pair(0, f + 1);
            uint64_t mid;
            while (i < f) {
                mid = (i + f) / 2;
                if (m_louds[mid] < val) {
                    i = mid + 1;
                } else {
                    f = mid;
                }
            }
            return std::make_pair(m_louds[i], i);
        }

        //return position, equal; where equal means that position contains val
        std::pair<uint64_t, bool> binary_search(uint32_t val, uint32_t i, uint32_t f) {
            if(m_louds[f] < val) return {f+1, false};
            uint32_t mid;
            while(i < f) {
                mid = (i+f) / 2;
                if(m_louds[mid] == val) return {mid, true};
                if(m_louds[mid] < val) {
                    i = mid + 1;
                }else {
                    f = mid;
                }
            }
            return {i, m_louds[i] == val};

        }


        void print() const {
            for (auto i = 0; i < m_louds.size(); ++i) {
                std::cout << (uint) m_louds.access(i).first << ", ";
            }
            std::cout << std::endl;

            for (auto i = 0; i < m_louds.size(); ++i) {
                std::cout << m_louds.access(i).second << ", ";
            }
            std::cout << std::endl;
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_louds.serialize(out, child, "louds");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_louds.load(in);
        }

    };
}
#endif