#ifndef CLTJ_UNCOMPACT_TRIE_V2_H
#define CLTJ_UNCOMPACT_TRIE_V2_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>

namespace cltj {

    class uncompact_trie {


    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        sdsl::int_vector<> m_ptr;
        sdsl::int_vector<> m_seq;

        void copy(const uncompact_trie &o) {
            m_ptr = o.m_ptr;
            m_seq = o.m_seq;
        }

        /*inline size_type rank0(const size_type i) const {
            return i - m_rank1(i);
        }*/

    public:

        const sdsl::int_vector<> &seq = m_seq;

        uncompact_trie() = default;

        uncompact_trie(const std::vector<uint32_t> &syms, const std::vector<size_type> &lengths) {
            m_ptr = sdsl::int_vector<>(syms.size()+1);
            m_seq = sdsl::int_vector<>(syms.size()+1);
            m_seq[0]=0;
            uint64_t pos_ptr = 0, pos_seq = 1;
            for(const auto &sym : syms) {
                m_seq[pos_seq] = sym;
                ++pos_seq;
            }
            pos_seq = 1;
            for(const auto &len : lengths) {
                m_ptr[pos_ptr] = pos_seq;
                ++pos_ptr;
                pos_seq += len;
            }
            m_ptr[pos_ptr] = pos_seq;
            m_ptr.resize(pos_ptr+1);

            sdsl::util::bit_compress(m_seq);
            sdsl::util::bit_compress(m_ptr);
        }

        //! Copy constructor
        uncompact_trie(const uncompact_trie &o) {
            copy(o);
        }

        //! Move constructor
        uncompact_trie(uncompact_trie &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        uncompact_trie &operator=(const uncompact_trie &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        uncompact_trie &operator=(uncompact_trie &&o) {
            if (this != &o) {
                m_ptr = std::move(o.m_ptr);
                m_seq = std::move(o.m_seq);
            }
            return *this;
        }

        void swap(uncompact_trie &o) {
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
        inline size_type child(uint32_t it, uint32_t n, uint32_t gap=1) const {
            return m_ptr[it-1+gap] + n -1;
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


        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
            if (m_seq[f] < val) return std::make_pair(0, f + 1);
            uint32_t mid;
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