#ifndef CLTJ_UNCOMPACT_TRIE_V2_H
#define CLTJ_UNCOMPACT_TRIE_V2_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <sdsl/vectors.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <cltj_regular_trie_v2.hpp>
#include <succ_support_v.hpp>

namespace cltj {

    class uncompact_trie_v2 {


    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        sdsl::int_vector<> m_ptr;
        sdsl::int_vector<> m_seq;

        void copy(const uncompact_trie_v2 &o) {
            m_ptr = o.m_ptr;
            m_seq = o.m_seq;
        }

        /*inline size_type rank0(const size_type i) const {
            return i - m_rank1(i);
        }*/

    public:

        const sdsl::int_vector<> &seq = m_seq;

        uncompact_trie_v2() = default;

        uncompact_trie_v2(const TrieV2 *trie, const uint64_t n_nodes) {
            m_ptr = sdsl::int_vector<>(n_nodes);
            m_seq = sdsl::int_vector<>(n_nodes);

            const TrieV2 *node;
            std::queue<const TrieV2 *> q;
            q.push(trie);
            m_seq[0]=0;
            uint64_t pos_seq = 1, pos_ptr = 0;
            while (!q.empty()) {
                node = q.front(); q.pop();
                auto children = node->get_children();
                m_ptr[pos_ptr] = pos_seq;
                ++pos_ptr;
                for (const auto &c: children) {
                    m_seq[pos_seq] = c;
                    ++pos_seq;
                    const TrieV2 *child = node->to_child(c);
                    if (!child->children.empty()) { //Check if it is a leaf
                        q.push(child);
                    }
                }
            }
            m_ptr[pos_ptr] = pos_seq;
            m_ptr.resize(pos_ptr+1);

            sdsl::util::bit_compress(m_seq);
            sdsl::util::bit_compress(m_ptr);

            for(size_type i = 0; i < m_seq.size(); ++i){
                std::cout << m_seq[i] << ", ";
            }
            std::cout << std::endl;

            for(size_type i = 0; i < m_ptr.size(); ++i){
                std::cout << m_ptr[i] << ", ";
            }
            std::cout << std::endl;

        }

        //! Copy constructor
        uncompact_trie_v2(const uncompact_trie_v2 &o) {
            copy(o);
        }

        //! Move constructor
        uncompact_trie_v2(uncompact_trie_v2 &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        uncompact_trie_v2 &operator=(const uncompact_trie_v2 &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        uncompact_trie_v2 &operator=(uncompact_trie_v2 &&o) {
            if (this != &o) {
                m_ptr = std::move(o.m_ptr);
                m_seq = std::move(o.m_seq);
            }
            return *this;
        }

        void swap(uncompact_trie_v2 &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr, o.m_ptr);
            std::swap(m_seq, o.m_seq);
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
        inline size_type child(uint32_t it, uint32_t n) const {
            return m_ptr[it] + n -1;
        }

        /*
            Receives index of node whos children we want to count
            Returns how many children said node has
        */
        inline size_type children(size_type i) const {
            return m_ptr[i+1] - m_ptr[i];
        }

        inline size_type first_child(size_type i) const {
            return m_ptr[i];
        }

        inline size_type nodeselect(size_type i) const {
            //std::cout << "i: " << i << "nodeselect: " << m_ptr[i] << std::endl;
            return i;
            //return i;
        }


        pair<uint32_t, uint32_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
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



        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_ptr.serialize(out, child, "ptr");
            written_bytes += m_seq.serialize(out, child, "seq");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_ptr.load(in);
            m_seq.load(in);
        }

    };
}
#endif