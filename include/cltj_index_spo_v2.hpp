//
// Created by Adrián on 19/10/23.
//

#ifndef CLTJ_INDEX_SPO_V2_HPP
#define CLTJ_INDEX_SPO_V2_HPP


#include <cltj_compact_trie_v3.hpp>
#include <cltj_uncompact_trie_v2.hpp>
#include <cltj_config.hpp>

namespace cltj {

    template<class Trie>
    class cltj_index_spo_v2 {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef Trie trie_type;

    private:
        std::array<trie_type, 6> m_tries;
        std::array<size_type, 3> m_gaps;

        TrieV2* create_trie(const vector<spo_triple> &D, const spo_order_type &order, size_type &n_nodes){
            TrieV2* root = new TrieV2();
            TrieV2* node;
            bool ok;
            for(const auto &spo: D){
                node = root;
                for(size_type i = 0; i < 3; ++i){
                    node = node->insert(spo[order[i]], ok);
                    if(ok) ++n_nodes;
                }
            }
            return root;
        }

        bool equal(const spo_triple &a, const spo_triple &b, const spo_order_type &order, const size_type l){
            for(size_type i = 0; i <= l; ++i){
                if(a[order[i]] != b[order[i]]) return false;
            }
            return true;
        }

        bool same_parent(const spo_triple &a, const spo_triple &b, const spo_order_type &order, const size_type l) {
            for(size_type i = 0; i < l; ++i){
                if(a[order[i]] != b[order[i]]) return false;
            }
            return true;
        }

        void sym_level(const vector<spo_triple> &D, const spo_order_type &order, size_type level,
                      std::vector<uint32_t> &syms, std::vector<size_type> &lengths){
            spo_triple prev, curr;
            size_type children;
            std::vector<size_type> res;
            for(uint32_t l = level; l < 3; ++l){
                children = 1;
                for(size_type i = 1; i < D.size(); ++i){
                    prev = D[i-1]; curr = D[i];
                    if(!equal(prev, curr, order, l)){
                        syms.emplace_back(prev[order[l]]);
                        if(!same_parent(prev, curr, order, l)) {// false when parent is the root
                            lengths.emplace_back(children);
                            children = 1;
                        }else {
                            ++children;
                        }
                    }
                }
                syms.emplace_back(prev[order[l]]);
                lengths.emplace_back(children);
            }
        }

        void copy(const cltj_index_spo_v2 &o){
            m_tries = o.m_tries;
            m_gaps = o.m_gaps;
        }

    public:

        const std::array<trie_type, 6> &tries = m_tries;
        const std::array<size_type, 3> &gaps = m_gaps;
        cltj_index_spo_v2() = default;

        cltj_index_spo_v2(vector<spo_triple> &D){

            for(size_type i = 0; i < 6; ++i){
                std::sort(D.begin(), D.end(), comparator_order(i));
                std::vector<uint32_t> syms;
                std::vector<size_type> lengths;
                sym_level(D,  spo_orders[i], i % 2, syms, lengths);
                if(i % 2 == 0){
                    m_gaps[i/2] = lengths[0];
                }
                m_tries[i] = trie_type(syms, lengths);
            }
        }

        //! Copy constructor
        cltj_index_spo_v2(const cltj_index_spo_v2 &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_spo_v2(cltj_index_spo_v2 &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_spo_v2 &operator=(const cltj_index_spo_v2 &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_spo_v2 &operator=(cltj_index_spo_v2 &&o) {
            if (this != &o) {
                m_tries = std::move(o.m_tries);
                m_gaps = std::move(o.m_gaps);
            }
            return *this;
        }

        void swap(cltj_index_spo_v2 &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
            std::swap(m_gaps, o.m_gaps);
        }

        inline trie_type* get_trie(size_type i){
            return &m_tries[i];
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            for(const auto & trie : m_tries){
                written_bytes += trie.serialize(out, child, "tries");
            }
            for(const auto & gap : m_gaps) {
                written_bytes += sdsl::write_member(gap, out, child, "gaps");
            }
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            for(auto & trie : m_tries){
                trie.load(in);
            }
            for(auto & gap : m_gaps){
                sdsl::read_member(gap, in);
            }
        }

    };

    typedef cltj::cltj_index_spo_v2<cltj::compact_trie_v3> compact_ltj;
    typedef cltj::cltj_index_spo_v2<cltj::uncompact_trie_v2> uncompact_ltj;

}

#endif //CLTJ_CLTJ_INDEX_ADRIAN_HPP
