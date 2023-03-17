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

#ifndef RING_LTJ_ITERATOR_UNIDIRECTIONAL_HPP
#define RING_LTJ_ITERATOR_UNIDIRECTIONAL_HPP

#define VERBOSE 0

#include <configuration.hpp>
#include <triple_pattern.hpp>
#include <bwt_interval.hpp>
#include <ltj_iterator_base.hpp>

namespace ring {

    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_unidirectional : public ltj_iterator_base< var_t, cons_t>{

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        typedef typename ring_type::bwt_type::wm_type wm_type;
        typedef struct {
            const wm_type* wm_ptr;
            range_type range;
        } wm_data_type;
        typedef  wt_range_iterator<typename ring_type::bwt_type::wm_type> wt_iterator_type;
        //std::vector<value_type> leap_result_type;

    private:
        const triple_pattern *m_ptr_triple_pattern;
        ring_type *m_ptr_ring; //TODO: should be const
        std::array<bwt_interval, 3> m_intervals;
        std::array<state_type, 3> m_state;
        size_type m_level = 0;
        bool m_is_empty = false;
        wt_iterator_type m_last_iterator;
        //std::stack<state_type> m_states;


        void copy(const ltj_iterator_unidirectional &o) {
            m_is_empty = o.m_is_empty;
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_ring = o.m_ptr_ring;
            m_intervals = o.m_intervals;
            m_last_iterator = o.m_last_iterator;
            m_state = o.m_state;
            m_level = o.m_level;
        }

    public:
        //const bool &is_empty = m_is_empty;
        const size_type& level = m_level;
        const std::array<state_type, 3>& state = m_state;

        ltj_iterator_unidirectional() = default;

        ltj_iterator_unidirectional(const triple_pattern *triple, ring_type *ring) {
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

                //Interval in O
                m_intervals[1] = m_ptr_ring->down_S(s_aux);
                auto o_aux = m_ptr_ring->next_O_in_S(m_intervals[1], m_ptr_triple_pattern->term_o.value);
                //Is the constant of O in m_i_o?
                if (o_aux != m_ptr_triple_pattern->term_o.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = o;

                //Interval in P
                m_intervals[2] = m_ptr_ring->down_S_O(m_intervals[1], o_aux);
                auto p_aux = m_ptr_ring->next_P_in_SO(m_intervals[2], m_ptr_triple_pattern->term_p.value);
                //Is the constant of P in m_i_p?
                if (p_aux != m_ptr_triple_pattern->term_p.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[2] = p;

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

                //Interval in S
                m_intervals[1] = m_ptr_ring->down_P(p_aux);
                auto s_aux = m_ptr_ring->next_S_in_P(m_intervals[1], m_ptr_triple_pattern->term_s.value);
                //Is the constant of O in m_i_o?
                if (s_aux != m_ptr_triple_pattern->term_s.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = s;

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

                //Interval in P
                m_intervals[1] = m_ptr_ring->down_O(o_aux);
                auto p_aux = m_ptr_ring->next_P_in_O(m_intervals[1], m_ptr_triple_pattern->term_p.value);
                //Is the constant of P in m_i_p?
                if (p_aux != m_ptr_triple_pattern->term_p.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = p;

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

                //Interval in O
                m_intervals[1] = m_ptr_ring->down_S(s_aux);
                auto o_aux = m_ptr_ring->next_O_in_S(m_intervals[1], m_ptr_triple_pattern->term_o.value);
                //Is the constant of O in m_i_o?
                if (o_aux != m_ptr_triple_pattern->term_o.value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = o;

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

                m_intervals[1] = m_ptr_ring->down_O(o_aux);
                m_level = 1;


            }
        }

        //! Copy constructor
        ltj_iterator_unidirectional(const ltj_iterator_unidirectional &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_unidirectional(ltj_iterator_unidirectional &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_unidirectional &operator=(const ltj_iterator_unidirectional &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_unidirectional &operator=(ltj_iterator_unidirectional &&o) {
            if (this != &o) {
                m_ptr_triple_pattern = std::move(o.m_ptr_triple_pattern);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_intervals = std::move(o.m_intervals);
                m_state = std::move(o.m_state);
                m_last_iterator = std::move(o.m_last_iterator);
                m_level = o.m_level;
                m_is_empty = o.m_is_empty;
            }
            return *this;
        }

        void swap(ltj_iterator_unidirectional &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr_triple_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_intervals, o.m_intervals);
            std::swap(m_state, o.m_state);
            std::swap(m_level, o.m_level);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_last_iterator, o.m_last_iterator);
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
                        m_intervals[2] = m_ptr_ring->down_S_P(m_intervals[1], c);
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
                        m_intervals[2] = m_ptr_ring->down_P_O(m_intervals[1], c);
                        m_state[m_level] = o;
                    }
                } else {
                    if (is_variable_subject(var)) {
                        m_intervals[2] = m_ptr_ring->down_O_S(m_intervals[1], c);
                        m_state[m_level] = s;
                    } else {
                        m_intervals[2] = m_ptr_ring->down_O_P(m_intervals[1], c);
                        m_state[m_level] = p;
                    }
                }
            }
            ++m_level;

        }

        void down(var_type var, size_type c, size_type k){
            down(var, c);
        };


        void up(var_type var) { //Go up in the trie
            if(m_level == 0) return;
            --m_level;
        };

        //TODO: para unidirectional no se necesitan (se podría usar todo esto en el iterator normal)
        value_type leap(var_type var) { //Return the minimum in the range
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
                        return m_ptr_ring->min_P_in_S(m_intervals[1]);
                    } else {
                        return m_ptr_ring->min_O_in_S(m_intervals[1]);
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->min_S_in_P(m_intervals[1]);
                    } else {
                        return m_ptr_ring->min_O_in_P(m_intervals[1]);
                    }
                } else {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->min_S_in_O(m_intervals[1]);
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
        };

        //TODO: para unidirectional no se necesitan (se podría usar todo esto en el iterator normal)
        value_type leap(var_type var, size_type c) { //Return the minimum in the range
            //0. Which term of our triple pattern is var
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
                        return m_ptr_ring->next_P_in_S(m_intervals[1], c);
                    } else {
                        return m_ptr_ring->next_O_in_S(m_intervals[1], c);
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->next_S_in_P(m_intervals[1], c);
                    } else {
                        return m_ptr_ring->next_O_in_P(m_intervals[1], c);
                    }
                } else {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->next_S_in_O(m_intervals[1], c);
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
        };

        inline wm_data_type get_wm_data(var_type var){
            range_type range{m_intervals[m_level].left(), m_intervals[m_level].right()};
            if(m_level == 0){
                if(is_variable_subject(var)){
                    return wm_data_type{&(m_ptr_ring->s_spo.get_wm()), range};
                }else if (is_variable_predicate(var)){
                    return wm_data_type{&(m_ptr_ring->p_spo.get_wm()), range};
                }else{
                    return wm_data_type{&(m_ptr_ring->o_spo.get_wm()), range};
                }
            }else if (m_level == 1){
                if (m_state[0] == s) {
                    if (is_variable_predicate(var)) {
                        return wm_data_type{&(m_ptr_ring->p_ops.get_wm()), range};
                    } else {
                        return wm_data_type{&(m_ptr_ring->o_spo.get_wm()), range};
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        return wm_data_type{&(m_ptr_ring->s_spo.get_wm()), range};
                    } else {
                        return wm_data_type{&(m_ptr_ring->o_ops.get_wm()), range};
                    }
                } else {
                    if (is_variable_subject(var)) {
                        return wm_data_type{&(m_ptr_ring->s_ops.get_wm()), range};
                    } else {
                        return wm_data_type{&(m_ptr_ring->p_spo.get_wm()), range};
                    }
                }
            }else{
                if (m_state[0] == s && m_state[1] == p) {
                    return wm_data_type{&(m_ptr_ring->o_ops.get_wm()), range};
                }else if(m_state[0] == p && m_state[1] == s){
                    return wm_data_type{&(m_ptr_ring->o_spo.get_wm()), range};
                }else if(m_state[0] == p && m_state[1] == o) {
                    return wm_data_type{&(m_ptr_ring->s_ops.get_wm()), range};
                }else if(m_state[0] == o && m_state[1] == p) {
                    return wm_data_type{&(m_ptr_ring->s_spo.get_wm()), range};
                }else if(m_state[0] == o && m_state[1] == s) {
                    return wm_data_type{&(m_ptr_ring->p_ops.get_wm()), range};
                }else{
                    return wm_data_type{&(m_ptr_ring->p_spo.get_wm()), range};
                }
            }
        }

        inline bool in_last_level(){
            return m_level == 2;
        }

        inline size_type interval_length() const{
            return m_intervals[m_level].size();
        }

        inline const bwt_interval& interval() const{
            return m_intervals[m_level];
        }

        //Solo funciona en último nivel, en otro caso habría que reajustar
        std::vector<uint64_t> seek_all(var_type var){
            if(m_state[0] == s){
                if(m_state[1] == p){
                    return m_ptr_ring->all_O_OPS_in_range(m_intervals[2]);
                }else{// m_state[1] == o
                    return m_ptr_ring->all_P_SPO_in_range(m_intervals[2]);
                }
            }else if (m_state[0] == p){
                if(m_state[1] == s){
                    return m_ptr_ring->all_O_SPO_in_range(m_intervals[2]);
                }else{//m_state[1] == o
                    return m_ptr_ring->all_S_OPS_in_range(m_intervals[2]);
                }
            }else{ //m_state[0] == o
                if(m_state[1] == s){
                    return m_ptr_ring->all_P_OPS_in_range(m_intervals[2]);
                }else{//m_state[1] == p
                    return m_ptr_ring->all_S_SPO_in_range(m_intervals[2]);
                }
            }
        }

        value_type seek_last(var_type var){
            range_type range = {m_intervals[2].left(), m_intervals[2].right()};
            if(m_state[0] == s){
                if(m_state[1] == p){
                    m_last_iterator = wt_iterator_type(&(m_ptr_ring->o_ops.get_wm()), range);
                    return m_last_iterator.next();
                }else{// m_state[1] == o
                    m_last_iterator = wt_iterator_type(&(m_ptr_ring->p_spo.get_wm()), range);
                    return m_last_iterator.next();
                }
            }else if (m_state[0] == p){
                if(m_state[1] == s){
                    m_last_iterator = wt_iterator_type(&(m_ptr_ring->o_spo.get_wm()), range);
                    return m_last_iterator.next();
                }else{//m_state[1] == o
                    m_last_iterator = wt_iterator_type(&(m_ptr_ring->s_ops.get_wm()), range);
                    return m_last_iterator.next();
                }
            }else{ //m_state[0] == o
                if(m_state[1] == s){
                    m_last_iterator = wt_iterator_type(&(m_ptr_ring->p_ops.get_wm()), range);
                    return m_last_iterator.next();
                }else{//m_state[1] == p
                    m_last_iterator = wt_iterator_type(&(m_ptr_ring->s_spo.get_wm()), range);
                    return m_last_iterator.next();
                }
            }
        }

        value_type seek_last_next(var_type var){
            return m_last_iterator.next();
        }

    };

}

#endif //RING_LTJ_ITERATOR_HPP