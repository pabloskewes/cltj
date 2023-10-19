//
// Created by Adrián on 19/10/23.
//

#ifndef CLTJ_CLTJ_INDEX_SPO_HPP
#define CLTJ_CLTJ_INDEX_SPO_HPP

#include <cltj_compact_trie.hpp>
#include <cltj_config.hpp>

namespace cltj {

    class cltj_index_spo {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        std::array<compact_trie, 6> m_tries;

        Trie* create_trie(const vector<spo_triple> &D, const std::array<size_type, 3> &order, size_type &n_nodes){
            Trie* root = new Trie();
            Trie* node;
            bool ok;
            for(const auto &spo: D){
                node = root;
                for(size_type i = 0; i < order.size(); ++i){
                    node = node->insert(spo[order[i]], ok);
                    if(ok) ++n_nodes;
                }
            }
            return root;
        }

        void copy(const cltj_index_spo &o){
            m_tries = o.m_tries;
        }

    public:

        const std::array<compact_trie, 6> &tries = m_tries;
        cltj_index_spo() = default;

        cltj_index_spo(const vector<spo_triple> &D){

            for(size_type i = 0; i < spo_orders.size(); ++i){
                size_type n_nodes = 1;
                Trie* trie = create_trie(D, spo_orders[i], n_nodes);
                m_tries[i] = compact_trie(trie, n_nodes);
                delete trie;
            }

        }

        //! Copy constructor
        cltj_index_spo(const cltj_index_spo &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_spo(cltj_index_spo &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_spo &operator=(const cltj_index_spo &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_spo &operator=(cltj_index_spo &&o) {
            if (this != &o) {
                m_tries = std::move(o.m_tries);
            }
            return *this;
        }

        void swap(cltj_index_spo &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
        }

        compact_trie* get_trie(size_type i){
            return &m_tries[i];
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            for(const auto & trie : m_tries){
                written_bytes += trie.serialize(out, child, "tries");
            }
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            for(auto & trie : m_tries){
                trie.load(in);
            }
        }

    };

}

#endif //CLTJ_CLTJ_INDEX_ADRIAN_HPP
