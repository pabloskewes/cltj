/*
 * ltj_iterator.hpp
 * Copyright (C) 2023 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LTJ_ITERATOR_HPP
#define LTJ_ITERATOR_HPP

#include <triple_pattern.hpp>
#include <cltj_config.hpp>
#include <vector>
#include <utils.hpp>
#include <string>
#define VERBOSE 0

namespace ltj {

    template<class index_scheme_t, class var_t, class cons_t>
    class ltj_iterator {//TODO: if CLTJ is eventually integrated with the ring to form a Compact Index Schemes project then this class has to be renamed to CLTJ_iterator, for instance.

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef index_scheme_t index_scheme_type;
        typedef uint64_t size_type;
        enum state_type {s, p, o};
        //std::vector<value_type> leap_result_type;


    private:
        const triple_pattern *m_ptr_triple_pattern;
        index_scheme_type *m_ptr_index; //TODO: should be const
        std::vector<std::string> m_orders = {"0 1 2", "0 2 1", "1 2 0", "1 0 2", "2 0 1", "2 1 0"};
        //Penso que con esto debería ser suficiente (mais parte do de Diego)
        bool m_is_empty = false;
        std::array<cltj::CTrie*, 6> m_tries;
        size_type m_nfixed = 0;
        std::array<state_type, 3> m_fixed;

        size_type m_trie_i = 0;
        size_type m_range_i = 0;
        std::array<std::vector<size_type>, 2> m_it_v = {std::vector<size_type>(4, 0),
                                                        std::vector<size_type>(4, 0)};
        //std::array<std::vector<size_type>, 2> m_pos_v = {std::vector<size_type>(4, 1),
        //                                                std::vector<size_type>(4, 1)}; //TODO: remove it
        std::array<std::vector<size_type>, 2> m_degree_v = {std::vector<size_type>(4, 1),
                                                            std::vector<size_type>(4, 1)};
        //TODO: add another vector for the last child
        //std::array<size_type, 2> m_parent_it_v = {0, 0}; // NEW DIEGO
        //std::array<size_type, 2> m_pos_in_parent_v = {1,1}; // NEW DIEGO

        void copy(const ltj_iterator &o) {
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_index = o.m_ptr_index;
            m_orders = o.m_orders;
            m_nfixed = o.m_nfixed;
            m_fixed = o.m_fixed;
            m_is_empty = o.m_is_empty;
            m_tries = o.m_tries;
            m_trie_i = o.m_trie_i;
            m_range_i = o.m_range_i;
            m_it_v = o.m_it_v;
            //m_pos_v = o.m_pos_v;
            m_degree_v = o.m_degree_v;
        }

    public:
        /*
            Returns the key of the current position of the iterator
        */
        const size_type &nfixed = m_nfixed;

        uint32_t key(){
            return m_tries[m_trie_i]->key_at(current());
        }

        inline bool is_variable_subject(var_type var) {
            return m_ptr_triple_pattern->term_s.is_variable && var == m_ptr_triple_pattern->term_s.value;
        }

        inline bool is_variable_predicate(var_type var) {
            return m_ptr_triple_pattern->term_p.is_variable && var == m_ptr_triple_pattern->term_p.value;
        }

        inline bool is_variable_object(var_type var) {
            return m_ptr_triple_pattern->term_o.is_variable && var == m_ptr_triple_pattern->term_o.value;
        }
        inline const bool is_empty(){
            return m_is_empty;
        }

        inline size_type parent(){
           return m_it_v[m_range_i][m_nfixed];
        }

        inline size_type current() const {
            return m_it_v[m_range_i][m_nfixed+1];
        }

        inline size_type nodemap(size_type i, cltj::CTrie* trie) const {
            return trie->b_rank0(i)-2;
        }

        inline size_type nodeselect(size_type i, cltj::CTrie* trie) const {
            return trie->b_sel0(i+2)+1;
        }


        ltj_iterator() = default;
        ltj_iterator(const triple_pattern *triple, index_scheme_type *index) {
            m_ptr_triple_pattern = triple;
            m_ptr_index = index;
            for(int i = 0; i < m_orders.size(); ++i){
                m_tries[i] = m_ptr_index->get_trie(m_orders[i]);
            }
            m_it_v[0][1] = 2;
            m_it_v[1][1] = 2;

            process_constants();
        }
        /*
        ~ltj_iterator(){
            m_ptr_triple_pattern = nullptr;
            m_ptr_index = nullptr;
        }
        */

        void process_constants(){

            if(!m_ptr_triple_pattern->s_is_variable() && !m_ptr_triple_pattern->o_is_variable()
                && !m_ptr_triple_pattern->p_is_variable()){

                m_range_i = 0;
                m_trie_i = 0;
                if(!exists(s, m_ptr_triple_pattern->term_s.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[0]=s;

                m_it_v[0][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[0][m_nfixed], 1);
                if(!exists(p, m_ptr_triple_pattern->term_p.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[1]=p;

                m_it_v[0][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[0][m_nfixed], 1);
                if(!exists(o, m_ptr_triple_pattern->term_o.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[2]=o;

            }else if (!m_ptr_triple_pattern->s_is_variable() && !m_ptr_triple_pattern->o_is_variable()){
                m_trie_i = 1;
                m_range_i = 1;

                if(!exists(s, m_ptr_triple_pattern->term_s.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[0]=s;
                m_it_v[m_range_i][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
                //m_pos_v[m_range_i][m_nfixed+1] = 1;

                if(!exists(o, m_ptr_triple_pattern->term_o.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[1]=o;
                m_it_v[m_range_i][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
                //m_pos_v[m_range_i][m_nfixed+1] = 1;


            }else if (!m_ptr_triple_pattern->s_is_variable() && !m_ptr_triple_pattern->p_is_variable()) {
                m_trie_i = 0;
                m_range_i = 0;

                if (!exists(s, m_ptr_triple_pattern->term_s.value)) {
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[0]=s;
                m_it_v[m_range_i][m_nfixed + 1]
                        = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
                //m_pos_v[m_range_i][m_nfixed + 1] = 1;

                if (!exists(p, m_ptr_triple_pattern->term_p.value)) {
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[1]=p;
                m_it_v[m_range_i][m_nfixed + 1]
                        = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
                //m_pos_v[m_range_i][m_nfixed + 1] = 1;


            }else if (!m_ptr_triple_pattern->p_is_variable() && !m_ptr_triple_pattern->o_is_variable()){
                m_trie_i = 2;
                m_range_i = 0;

                if(!exists(s, m_ptr_triple_pattern->term_s.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[0]=p;
                m_it_v[m_range_i][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
                //m_pos_v[m_range_i][m_nfixed+1] = 1;

                if(!exists(o, m_ptr_triple_pattern->term_o.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[1]=o;
                m_it_v[m_range_i][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
                //m_pos_v[m_range_i][m_nfixed+1] = 1;


            }else if (!m_ptr_triple_pattern->s_is_variable()){
                m_trie_i = 0;
                m_range_i = 0;
                if(!exists(s, m_ptr_triple_pattern->term_s.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[0]=s;
                m_it_v[0][m_nfixed+1] = m_it_v[1][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[0][m_nfixed], 1);
                //m_pos_v[0][m_nfixed+1]  = m_pos_v[1][m_nfixed+1] = 1;


            }else if (!m_ptr_triple_pattern->p_is_variable()){
                m_trie_i = 2;
                m_range_i = 0;
                if(!exists(p, m_ptr_triple_pattern->term_p.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[0]=p;
                m_it_v[0][m_nfixed+1] = m_it_v[1][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[0][m_nfixed], 1);
               // m_pos_v[0][m_nfixed+1]  = m_pos_v[1][m_nfixed+1] = 1;

            }else if (!m_ptr_triple_pattern->o_is_variable()){
                m_trie_i = 4;
                m_range_i = 0;
                if(!exists(o, m_ptr_triple_pattern->term_o.value)){
                    m_is_empty = true;
                    return;
                }
                ++m_nfixed;
                m_fixed[0]=o;
                m_it_v[0][m_nfixed+1] = m_it_v[1][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[0][m_nfixed], 1);
               // m_pos_v[0][m_nfixed+1]  = m_pos_v[1][m_nfixed+1] = 1;
            }
        }

        const triple_pattern* get_triple_pattern() const{
            return m_ptr_triple_pattern;
        }
        //! Copy constructor
        ltj_iterator(const ltj_iterator &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator(ltj_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator &operator=(const ltj_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator &operator=(ltj_iterator &&o) {
            if (this != &o) {
                m_ptr_triple_pattern = std::move(o.m_ptr_triple_pattern);
                m_ptr_index = std::move(o.m_ptr_index);
                m_orders = std::move(o.m_orders);
                m_nfixed = std::move(o.m_nfixed);
                m_fixed = std::move(o.m_fixed);
                m_is_empty = std::move(o.m_is_empty);
                m_tries = std::move(o.m_tries);
                m_trie_i = std::move(o.m_trie_i);
                m_range_i = std::move(o.m_range_i);
                m_it_v = std::move(o.m_it_v);
                //m_pos_v = std::move(o.m_pos_v);
                m_degree_v = std::move(o.m_degree_v);
            }
            return *this;
        }

        void swap(ltj_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr_triple_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_index, o.m_ptr_index);
            std::swap(m_orders, o.m_orders);
            std::swap(m_nfixed, o.m_nfixed);
            std::swap(m_fixed, o.m_fixed);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_tries, o.m_tries);
            std::swap(m_trie_i, o.m_trie_i);
            std::swap(m_range_i, o.m_range_i);
            std::swap(m_it_v, o.m_it_v);
            //std::swap(m_pos_v, o.m_pos_v);
            std::swap(m_degree_v, o.m_degree_v);
        }



        void down(var_type var, value_type c) { //Go down in the trie
            ++m_nfixed;
            state_type state;
            if (is_variable_subject(var)) {
                state = s;
            } else if (is_variable_predicate(var)) {
                state = p;
            } else {
                state = o;
            }
            m_fixed[m_nfixed-1] = state;
            if(m_nfixed == 1){ //First fix
                //if(m_tries[m_trie_i]->childrenCount(m_it_v[0][m_nfixed])){
                auto cnt = m_tries[m_trie_i]->childrenCount(m_it_v[0][m_nfixed]);
                m_it_v[0][m_nfixed+1] = m_it_v[1][m_nfixed+1]
                        = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
                //m_pos_v[0][m_nfixed+1] = m_pos_v[1][m_nfixed+1] = 1;
                m_degree_v[0][m_nfixed+1] = m_degree_v[1][m_nfixed+1] = cnt;
                //}
            }else if (m_nfixed == 2){ //Fix one trie (the active one)
                auto cnt = m_tries[m_trie_i]->childrenCount(m_it_v[m_range_i][m_nfixed]);
                m_it_v[m_range_i][m_nfixed+1] = m_tries[m_trie_i]->child(m_it_v[m_range_i][m_nfixed], 1);
               // m_pos_v[m_range_i][m_nfixed+1] = 1;
                m_degree_v[m_range_i][m_nfixed+1] = cnt;
                //}
            }
        };

        //Reverses the intervals and variable weights. Also resets the current value.
        void up(var_type var){ //Go up in the trie
            --m_nfixed;
        };


        //TODO update degrees
        bool exists(state_type state, size_type c) { //Return the minimum in the range
            //If c=-1 we need to get the minimum value for the current level.

            if(m_nfixed == 0) {
                if (state == s) {
                    m_trie_i = 0;
                } else if (state == p) {
                    m_trie_i = 2;
                } else {
                    m_trie_i = 4;
                }
            }else if (m_nfixed == 1){
                if (state == s) { //Fix variables
                    m_trie_i = (m_fixed[m_nfixed-1] == o) ? 4 : 3 ;
                    m_range_i = (m_fixed[m_nfixed-1] == o) ? 0 : 1;
                } else if (state == p) {
                    m_trie_i = (m_fixed[m_nfixed-1] == s) ? 0 : 5 ;
                    m_range_i = (m_fixed[m_nfixed-1] == s) ? 0 : 1;
                } else {
                    m_trie_i = (m_fixed[m_nfixed-1] == p) ? 2 : 1 ;
                    m_range_i = (m_fixed[m_nfixed-1] == p) ? 0 : 1;
                }
            }

            auto trie = m_tries[m_trie_i];
            //auto it_parent = parent();
            auto it = current();
            uint32_t cnt = trie->childrenCount(it);
            auto first_child = trie->child(it, 1);
            uint32_t beg = nodemap(first_child, trie);
            uint32_t end = beg+cnt-1;
            auto p = trie->binary_search_seek(c, beg, end);

            if(p.second > end or p.first != c) return false;

            m_it_v[0][m_nfixed+1] = m_it_v[1][m_nfixed+1]  = nodeselect(p.second, trie); //next pos in the trie
            m_degree_v[0][m_nfixed+1] = m_degree_v[1][m_nfixed+1] = cnt;
            //m_pos_v[m_range_i][m_nfixed+1] = trie->child(it_parent, cnt);
            return true;
        }

        value_type leap(var_type var, size_type c = -1) { //Return the minimum in the range
            //If c=-1 we need to get the minimum value for the current level.

            if(m_nfixed == 0) {
                if (is_variable_subject(var)) {
                    m_trie_i = 0;
                } else if (is_variable_predicate(var)) {
                    m_trie_i = 2;
                } else {
                    m_trie_i = 4;
                }
            }else if (m_nfixed == 1){
                if (is_variable_subject(var)) { //Fix variables
                    m_trie_i = (m_fixed[m_nfixed-1] == o) ? 4 : 3 ;
                    m_range_i = (m_fixed[m_nfixed-1] == o) ? 0 : 1;
                } else if (is_variable_predicate(var)) {
                    m_trie_i = (m_fixed[m_nfixed-1] == s) ? 0 : 5 ;
                    m_range_i = (m_fixed[m_nfixed-1] == s) ? 0 : 1;
                } else {
                    m_trie_i = (m_fixed[m_nfixed-1] == p) ? 2 : 1 ;
                    m_range_i = (m_fixed[m_nfixed-1] == p) ? 0 : 1;
                }
            }


            if (c == -1) return key();
            auto trie = m_tries[m_trie_i];
            auto it_parent = parent();
            auto cnt = m_degree_v[m_range_i][m_nfixed];
            //uint32_t i = trie->b_rank0(current())-2;
            auto beg = nodemap(current(), trie);
            auto end = nodemap(trie->child(it_parent, cnt), trie); //TODO: save it as cnt
            auto p  = trie->binary_search_seek(c, beg, end);

            if(p.second > end) return 0;

            m_it_v[m_range_i][m_nfixed+1]  = nodeselect(p.second, trie); //next pos in the trie
            return p.first;
        }


        bool in_last_level(){
            return m_nfixed == 2;
        }

        inline size_type children() const{
            return m_degree_v[m_range_i][m_nfixed+1];
        }

        inline size_type subtree_size_d1() const { //TODO: review this
            auto trie = m_tries[m_trie_i];
            auto cnt = m_degree_v[m_range_i][m_nfixed+1];
            size_type leftmost_leaf, rightmost_leaf;
            auto it = current();
            //Leftmost
            leftmost_leaf = trie->child(trie->child(it, 1), 1);
            //Rightmost
            it = trie->child(it, cnt);
            cnt = trie->childrenCount(it);
            rightmost_leaf = trie->child(it, cnt);
            return rightmost_leaf - leftmost_leaf + 1;
        }



        std::vector<uint64_t> seek_all(var_type x_j){
            std::vector<uint64_t> results;
            auto trie = m_tries[m_trie_i];
            auto it_parent = parent();
            uint32_t cnt = trie->childrenCount(it_parent);
            size_type it;
            for(int i = 1; i <= cnt; ++i){
                it = trie->child(it_parent, i);
                results.emplace_back(trie->key_at(it));
            }
            return results;
        }
    };

}

#endif //LTJ_ITERATOR_HPP
