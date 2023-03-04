/*
 * ltj_iterator.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
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

#ifndef RING_LTJ_ITERATOR_HPP
#define RING_LTJ_ITERATOR_HPP

#define VERBOSE 0

#include <ltj_iterator_base.hpp>

namespace ring {

    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator : public ltj_iterator_base< var_t, cons_t>{

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        enum state_type {s, p, o};
        //std::vector<value_type> leap_result_type;

    private:
        const triple_pattern *m_ptr_triple_pattern;
        ring_type *m_ptr_ring; //TODO: should be const
        std::array<bwt_interval, 3> m_intervals;
        std::array<value_type, 3> m_consts;
        std::array<state_type, 3> m_state;
        size_type m_level = 0;
        bool m_is_empty = false;
        //std::stack<state_type> m_states;


        void copy(const ltj_iterator &o) {
            m_is_empty = o.m_is_empty;
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_ring = o.m_ptr_ring;
            m_intervals = o.m_intervals;
            m_state = o.m_state;
            m_level = o.m_level;
            m_consts = o.m_consts;
        }

    public:
        //const bool &is_empty = m_is_empty;

        ltj_iterator() = default;

        ltj_iterator(const triple_pattern *triple, ring_type *ring) {
            m_ptr_triple_pattern = triple;
            m_ptr_ring = ring;
            m_intervals[0] = m_ptr_ring->open_POS();
            //Init current values and intervals according to the triple
            if (!m_ptr_triple_pattern->s_is_variable() && !m_ptr_triple_pattern->p_is_variable()
                && !m_ptr_triple_pattern->o_is_variable()) {
                //S->O->P to avoid forward steps

                //Interval in S
                auto s_aux = m_ptr_ring->next_S(m_intervals[0], m_ptr_triple_pattern->term_s.value);
                //Is the constant of S in m_i_s?
                if (s_aux != m_ptr_triple_pattern->term_s.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = s_aux;

                //Interval in O
                m_intervals[1] = m_ptr_ring->down_S(s_aux);
                auto o_aux = m_ptr_ring->next_O_in_S(m_intervals[1], m_ptr_triple_pattern->term_o.value);
                //Is the constant of O in m_i_o?
                if (o_aux != m_ptr_triple_pattern->term_o.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = o;
                m_consts[1] = o_aux;

                //Interval in P
                m_intervals[2] = m_ptr_ring->down_S_O(m_intervals[1], o_aux);
                auto p_aux = m_ptr_ring->next_P_in_SO(m_intervals[2], m_ptr_triple_pattern->term_p.value);
                //Is the constant of P in m_i_p?
                if (p_aux != m_ptr_triple_pattern->term_p.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[2] = p;
                m_consts[2] = p_aux;

                m_level = 3;

            } else if (!m_ptr_triple_pattern->s_is_variable() && !m_ptr_triple_pattern->p_is_variable()) {
                //P->S to avoid forward steps

                //Interval in P
                auto p_aux = m_ptr_ring->next_P(m_intervals[0], m_ptr_triple_pattern->term_p.value);
                //Is the constant of S in m_i_s?
                if (p_aux != m_ptr_triple_pattern->term_p.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = p;
                m_consts[0] = p_aux;

                //Interval in S
                m_intervals[1] = m_ptr_ring->down_P(p_aux);
                auto s_aux = m_ptr_ring->next_S_in_P(m_intervals[1], m_ptr_triple_pattern->term_s.value);
                //Is the constant of O in m_i_o?
                if (s_aux != m_ptr_triple_pattern->term_s.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = s;
                m_consts[1] = s_aux;

                //Interval in O
                m_intervals[2] = m_ptr_ring->down_P_S(m_intervals[1], s_aux);
                m_level = 2;

            } else if (!m_ptr_triple_pattern->p_is_variable() && !m_ptr_triple_pattern->o_is_variable()) {
                //O->P to avoid forward steps

                //Interval in O
                auto o_aux = m_ptr_ring->next_O(m_intervals[0], m_ptr_triple_pattern->term_o.value);
                //Is the constant of S in m_i_s?
                if (o_aux != m_ptr_triple_pattern->term_o.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = o;
                m_consts[0] = o_aux;

                //Interval in P
                m_intervals[1] = m_ptr_ring->down_O(o_aux);
                auto p_aux = m_ptr_ring->next_P_in_O(m_intervals[1], m_ptr_triple_pattern->term_p.value);
                //Is the constant of P in m_i_p?
                if (p_aux != m_ptr_triple_pattern->term_p.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = p;
                m_consts[1] = p_aux;

                //Interval in S
                m_intervals[2] = m_ptr_ring->down_O_P(m_intervals[1], p_aux);
                m_level = 2;

            } else if (!m_ptr_triple_pattern->s_is_variable() && !m_ptr_triple_pattern->o_is_variable()) {
                //S->O to avoid forward steps

                //Interval in S
                auto s_aux = m_ptr_ring->next_S(m_intervals[0], m_ptr_triple_pattern->term_s.value);
                //Is the constant of S in m_i_s?
                if (s_aux != m_ptr_triple_pattern->term_s.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = s_aux;

                //Interval in O
                m_intervals[1] = m_ptr_ring->down_S(s_aux);
                auto o_aux = m_ptr_ring->next_O_in_S(m_intervals[1], m_ptr_triple_pattern->term_o.value);
                //Is the constant of O in m_i_o?
                if (o_aux != m_ptr_triple_pattern->term_o.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = o;
                m_consts[1] = o_aux;

                //Interval in O
                m_intervals[2] = m_ptr_ring->down_S_O(m_intervals[1], o_aux);
                m_level = 2;

            } else if (!m_ptr_triple_pattern->s_is_variable()) {

                //Interval in S
                auto s_aux = m_ptr_ring->next_S(m_intervals[0], m_ptr_triple_pattern->term_s.value);
                //Is the constant of S in m_i_s?
                if (s_aux != m_ptr_triple_pattern->term_s.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = s_aux;

                m_intervals[1] = m_ptr_ring->down_S(s_aux);
                m_level = 1;

            } else if (!m_ptr_triple_pattern->p_is_variable()) {

                //Interval in P
                auto p_aux = m_ptr_ring->next_P(m_intervals[0], m_ptr_triple_pattern->term_p.value);
                //Is the constant of P in m_i_p?
                if (p_aux != m_ptr_triple_pattern->term_p.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = p;
                m_consts[0] = p_aux;

                m_intervals[1] = m_ptr_ring->down_P(p_aux);
                m_level = 1;

            } else if (!m_ptr_triple_pattern->o_is_variable()) {

                //Interval in O
                auto o_aux = m_ptr_ring->next_O(m_intervals[0], m_ptr_triple_pattern->term_o.value);
                //Is the constant of P in m_i_p?
                if (o_aux != m_ptr_triple_pattern->term_o.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = o;
                m_consts[0] = o_aux;

                m_intervals[1] = m_ptr_ring->down_O(o_aux);
                m_level = 1;


            }
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
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_intervals = std::move(o.m_intervals);
                m_consts = std::move(o.m_consts);
                m_state = std::move(o.m_state);
                m_level = o.m_level;
                m_is_empty = o.m_is_empty;
            }
            return *this;
        }

        void swap(ltj_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr_triple_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_intervals, o.m_intervals);
            std::swap(m_consts, o.m_consts);
            std::swap(m_state, o.m_state);
            std::swap(m_level, o.m_level);
            std::swap(m_is_empty, o.m_is_empty);
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


        inline bool is_empty(){
            return m_is_empty;
        }

        void down(var_type var, size_type c) { //Go down in the trie
            if (m_level > 2) return;
            if (m_level == 0) {
                if (is_variable_subject(var)) {
                    m_intervals[1] = m_ptr_ring->down_S(c);
                    m_state[m_level] = s;
                } else if (is_variable_predicate(var)) {
                    m_intervals[1] = m_ptr_ring->down_P(c);
                    m_state[m_level] = p;
                } else {
                    m_intervals[1] = m_ptr_ring->down_O(c);
                    m_state[m_level] = o;
                }
            } else if(m_level == 1) {//m_level = 1
                if (m_state[0] == s) {
                    if (is_variable_predicate(var)) {
                        m_intervals[2] = m_ptr_ring->down_S_P(m_intervals[1], m_consts[0], c);
                        m_state[m_level] = p;
                    } else {
                        m_intervals[2] = m_ptr_ring->down_S_O(m_intervals[1], c);
                        m_state[m_level] = o;
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        m_intervals[2] = m_ptr_ring->down_P_S(m_intervals[1], c);
                        m_state[m_level] = s;
                    } else {
                        m_intervals[2] = m_ptr_ring->down_P_O(m_intervals[1], m_consts[0], c);
                        m_state[m_level] = o;
                    }
                } else {
                    if (is_variable_subject(var)) {
                        m_intervals[2] = m_ptr_ring->down_O_S(m_intervals[1], m_consts[0], c);
                        m_state[m_level] = s;
                    } else {
                        m_intervals[2] = m_ptr_ring->down_O_P(m_intervals[1], c);
                        m_state[m_level] = p;
                    }
                }
            }
            m_consts[m_level] = c;
            ++m_level;
        };

        void down(var_type var, size_type c, size_type k){
            down(var, c);
        };


        void up(var_type var) { //Go up in the trie
            if(m_level == 0) return;
            --m_level;
        };

        value_type
        leap(var_type var) { //Return the minimum in the range
            //0. Which term of our triple pattern is var
            if(m_level == 0){
                if(is_variable_subject(var)){
                    return m_ptr_ring->min_S(m_intervals[0]);
                }else if (is_variable_predicate(var)){
                    return m_ptr_ring->min_P(m_intervals[0]);
                }else{
                    return m_ptr_ring->min_O(m_intervals[0]);
                }
            }else if (m_level == 1){
                if (m_state[0] == s) {
                    if (is_variable_predicate(var)) {
                        return m_ptr_ring->min_P_in_S(m_intervals[1], m_consts[0]);
                    } else {
                        return m_ptr_ring->min_O_in_S(m_intervals[1]);
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->min_S_in_P(m_intervals[1]);
                    } else {
                        return m_ptr_ring->min_O_in_P(m_intervals[1], m_consts[0]);
                    }
                } else {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->min_S_in_O(m_intervals[1], m_consts[0]);
                    } else {
                        return m_ptr_ring->min_P_in_O(m_intervals[1]);
                    }
                }
            }else{
                if (m_state[0] == s && m_state[1] == p) {
                    return m_ptr_ring->min_O_in_SP(m_intervals[2]);
                }else if(m_state[0] == p && m_state[1] == s){
                    return m_ptr_ring->min_O_in_PS(m_intervals[2]);
                }else if(m_state[0] == p && m_state[1] == o) {
                    return m_ptr_ring->min_S_in_PO(m_intervals[2]);
                }else if(m_state[0] == o && m_state[1] == p) {
                    return m_ptr_ring->min_S_in_OP(m_intervals[2]);
                }else if(m_state[0] == o && m_state[1] == s) {
                    return m_ptr_ring->min_P_in_OS(m_intervals[2]);
                }else{
                    return m_ptr_ring->min_P_in_SO(m_intervals[2]);
                }
            }
            return 0;
        };

        value_type leap(var_type var, size_type c) { //Return the next value greater or equal than c in the range
            if(m_level == 0){
                if(is_variable_subject(var)){
                    return m_ptr_ring->next_S(m_intervals[0], c);
                }else if (is_variable_predicate(var)){
                    return m_ptr_ring->next_P(m_intervals[0], c);
                }else{
                    return m_ptr_ring->next_O(m_intervals[0], c);
                }
            }else if (m_level == 1){
                if (m_state[0] == s) {
                    if (is_variable_predicate(var)) {
                        return m_ptr_ring->next_P_in_S(m_intervals[1], m_consts[0], c);
                    } else {
                        return m_ptr_ring->next_O_in_S(m_intervals[1], c);
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->next_S_in_P(m_intervals[1], c);
                    } else {
                        return m_ptr_ring->next_O_in_P(m_intervals[1], m_consts[0], c);
                    }
                } else {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->next_S_in_O(m_intervals[1], m_consts[0], c);
                    } else {
                        return m_ptr_ring->next_P_in_O(m_intervals[1], c);
                    }
                }
            }else{
                if (m_state[0] == s && m_state[1] == p) {
                    return m_ptr_ring->next_O_in_SP(m_intervals[2], c);
                }else if(m_state[0] == p && m_state[1] == s){
                    return m_ptr_ring->next_O_in_PS(m_intervals[2], c);
                }else if(m_state[0] == p && m_state[1] == o) {
                    return m_ptr_ring->next_S_in_PO(m_intervals[2], c);
                }else if(m_state[0] == o && m_state[1] == p) {
                    return m_ptr_ring->next_S_in_OP(m_intervals[2], c);
                }else if(m_state[0] == o && m_state[1] == s) {
                    return m_ptr_ring->next_P_in_OS(m_intervals[2], c);
                }else{
                    return m_ptr_ring->next_P_in_SO(m_intervals[2], c);
                }
            }
            return 0;
        }

        inline bool in_last_level(){
            return m_level == 2;
        }

        inline size_type interval_length() const{
            return m_intervals[m_level].size();
        }

        //Solo funciona en último nivel, en otro caso habría que reajustar
        std::vector<uint64_t> seek_all(var_type var){
            if (is_variable_subject(var)){
                return m_ptr_ring->all_S_in_range(m_intervals[2]);
            }else if (is_variable_predicate(var)){
                return m_ptr_ring->all_P_in_range(m_intervals[2]);
            }else if (is_variable_object(var)){
                return m_ptr_ring->all_O_in_range(m_intervals[2]);
            }
            return {};
        }


    };

}

#endif //RING_LTJ_ITERATOR_HPP