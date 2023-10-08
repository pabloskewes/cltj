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
        enum state_type {s = 0, p = 1, o = 2};
        typedef struct {
            std::array<size_type, 2> it;
            size_type cnt;
            size_type beg;
            size_type end;
        } level_data_type;
        typedef std::array<level_data_type, 4> status_type;
        typedef std::array<bool, 4> redo_array_type;
        //std::vector<value_type> leap_result_type;


    private:
        const triple_pattern *m_ptr_triple_pattern;
        index_scheme_type *m_ptr_index; //TODO: should be const
        std::vector<std::string> m_orders = {"0 1 2", "0 2 1", "1 2 0", "1 0 2", "2 0 1", "2 1 0"};
        //                                     SPO      SOP      POS      PSO     OSP      OPS
        //Penso que con esto debería ser suficiente (mais parte do de Diego)
        bool m_is_empty = false;
        std::array<cltj::CTrie*, 6> m_tries;
        size_type m_nfixed = 0;
        std::array<state_type, 3> m_fixed;
        
        size_type m_trie_i = 0;
        size_type m_status_i = 0;
        status_type m_status;
        redo_array_type m_redo;
        
        
        //std::array<std::vector<size_type>, 2> m_it_v = {std::vector<size_type>(4, 0),
        //                                                std::vector<size_type>(4, 0)};
        //std::array<std::vector<size_type>, 2> m_pos_v = {std::vector<size_type>(4, 1),
        //                                                std::vector<size_type>(4, 1)}; //TODO: remove it
        //std::array<std::vector<size_type>, 2> m_degree_v = {std::vector<size_type>(4, 1),
        //                                                    std::vector<size_type>(4, 1)};
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
            m_status_i = o.m_status_i;
            m_status = o.m_status;
            m_redo = o.m_redo;
        }

        void print_status(){
            std::cout << "fixed: " << m_nfixed << std::endl;
            for(int i = 0; i < m_status.size(); ++i){
                std::cout << "it0=" << m_status[i].it[0] << " it1=" << m_status[i].it[1] << " cnt=" << m_status[i].cnt
                << " beg=" << m_status[i].beg << " end=" << m_status[i].end << std::endl;
            }
        }

        void print_redo(){
            for(int i = 0; i < m_redo.size(); ++i){
                std::cout << m_redo[i] << " ";
            }
            std::cout << std::endl;
        }

        void process_constants(){

            if(!m_ptr_triple_pattern->s_is_variable()){
                if(!exists(s, m_ptr_triple_pattern->term_s.value)){
                    m_is_empty = true;
                    return;
                }
                down(s);
            }

            if(!m_ptr_triple_pattern->p_is_variable()){
                if(!exists(p, m_ptr_triple_pattern->term_p.value)){
                    m_is_empty = true;
                    return;
                }
                down(p);
            }

            if(!m_ptr_triple_pattern->o_is_variable()){
                if(!exists(o, m_ptr_triple_pattern->term_o.value)){
                    m_is_empty = true;
                    return;
                }
                down(o);
            }

            std::cout << "Constants" << std::endl;
            //print_status();
        }

        void choose_trie(state_type state){
            if(m_nfixed == 0) {
                m_trie_i = 2*state;
            }else if (m_nfixed == 1){
                if (state == s) { //Fix variables
                    m_trie_i = (m_fixed[m_nfixed-1] == o) ? 4 : 3 ;
                    m_status_i = (m_fixed[m_nfixed-1] == o) ? 0 : 1;
                } else if (state == p) {
                    m_trie_i = (m_fixed[m_nfixed-1] == s) ? 0 : 5 ;
                    m_status_i = (m_fixed[m_nfixed-1] == s) ? 0 : 1;
                } else {
                    m_trie_i = (m_fixed[m_nfixed-1] == p) ? 2 : 1 ;
                    m_status_i = (m_fixed[m_nfixed-1] == p) ? 0 : 1;
                }
            }
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

        inline size_type parent() const{
           return m_status[m_nfixed].it[m_status_i];
        }

        inline size_type current() const {
            return m_status[m_nfixed+1].it[m_status_i];
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
                /*std::cout << "Trie[" << i << "]: ";
                for(int j = 0; j < m_tries[i]->B.size(); ++j){
                    if(m_tries[i]->B[j]){
                        std::cout << "1";
                    }else{
                        std::cout << "0";
                    }
                }
                std::cout << std::endl;*/
            }
            m_status[0].it[0] = 2;
            m_status[0].it[1] = 2;
            m_status[0].beg = 2;
            m_status[0].end = 1;
            m_status[0].cnt = 1;
            //m_status[1][0].it = 2;
            //m_status[1][0].last = 1;
            m_redo[0] = true;
            m_redo[1] = true;
            m_redo[2] = true;
            m_redo[3] = true;

            process_constants();
        }
        /*
        ~ltj_iterator(){
            m_ptr_triple_pattern = nullptr;
            m_ptr_index = nullptr;
        }
        */



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
                m_status_i = std::move(o.m_status_i);
                m_status = std::move(o.m_status);
                m_redo = std::move(o.m_redo);
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
            std::swap(m_status_i, o.m_status_i);
            std::swap(m_status, o.m_status);
            std::swap(m_redo, o.m_redo);
        }


        void down(state_type state){
            ++m_nfixed;
            std::cout << "n_fixed: " << m_nfixed << std::endl;
            m_fixed[m_nfixed-1] = state;
            m_redo[m_nfixed] = true;
            print_redo();
        }

        void down(var_type var, value_type c) { //Go down in the trie
            state_type state;
            if (is_variable_subject(var)) {
                state = s;
            } else if (is_variable_predicate(var)) {
                state = p;
            } else {
                state = o;
            }
            down(state);
        };

        //Reverses the intervals and variable weights. Also resets the current value.
        void up(var_type var){ //Go up in the trie
            --m_nfixed;
        };



        bool exists(state_type state, size_type c) { //Return the minimum in the range

            choose_trie(state);
            auto trie = m_tries[m_trie_i];

            size_type beg, end;
            auto cnt = trie->childrenCount(parent());
            beg = nodemap(trie->child(parent(), 1), trie);
            end = beg + cnt -1;
            auto p = trie->binary_search_seek(c, beg, end);
            if(p.second > end or p.first != c) return false;
            m_status[m_nfixed+1].beg = beg;
            m_status[m_nfixed+1].end = end;
            m_status[m_nfixed+1].cnt = cnt;
            m_redo[m_nfixed] = false;
            if(m_nfixed == 0){
                m_status[m_nfixed+1].it[0] = nodeselect(p.second, trie);
                m_status[m_nfixed+1].it[1] = nodeselect(p.second, m_tries[m_trie_i+1]);
            }else{
                m_status[m_nfixed+1].it[m_status_i] = nodeselect(p.second, trie);
            }
            //print_status();
            return true;
        }

        value_type leap(var_type var, size_type c = -1) { //Return the minimum in the range
            //If c=-1 we need to get the minimum value for the current level.


            state_type state = o;
            if (is_variable_subject(var)) {
                state = s;
            } else if (is_variable_predicate(var)) {
                state = p;
            }
            choose_trie(state);
            std::cout << "trie_i: " << m_trie_i << " status_i: " << m_status_i << std::endl;
            cltj::CompactTrieIV* trie = m_tries[m_trie_i];
            size_type beg, end, it;
            std::cout << "Leap redo" << std::endl;
            print_redo();
            if(m_redo[m_nfixed]){ //First time of leap (after a down)
                std::cout << "Redoing" << std::endl;
                auto cnt = trie->childrenCount(parent());
                it = trie->child(parent(), 1);
                beg = nodemap(it, trie);
                end = beg + cnt -1;
                m_status[m_nfixed+1].beg = beg;
                m_status[m_nfixed+1].end = end;
                m_status[m_nfixed+1].it[0] = m_status[m_nfixed+1].it[1] = it;
                m_status[m_nfixed+1].cnt  = cnt;
                m_redo[m_nfixed] = false;
            }else{
                beg = nodemap(current(), trie);
                end = m_status[m_nfixed+1].end;
            }
            if(c == -1) return key();
            auto p  = trie->binary_search_seek(c, beg, end);
            if(p.second > end) return 0;

            if(m_nfixed == 0){
                m_status[m_nfixed+1].it[0] = nodeselect(p.second, trie);
                m_status[m_nfixed+1].it[1] = nodeselect(p.second, m_tries[m_trie_i+1]);
            }else{
                m_status[m_nfixed+1].it[m_status_i] = nodeselect(p.second, trie);
            }
            //print_status();
            return p.first;
        }


        bool in_last_level(){
            return m_nfixed == 2;
        }

        inline size_type children() const{
            return m_status[m_nfixed+1].cnt;
        }

        inline size_type subtree_size_d1() const { //TODO: review this
            auto trie = m_tries[m_trie_i];
            auto cnt = m_status[m_nfixed+1].cnt;
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
            for(int i = 1; i <= cnt; ++i){ //TODO: improve this avoiding computing the child every time
                it = trie->child(it_parent, i);
                results.emplace_back(trie->key_at(it));
            }
            return results;
        }
    };

}

#endif //LTJ_ITERATOR_HPP
