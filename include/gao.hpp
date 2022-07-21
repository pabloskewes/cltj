/***
BSD 2-Clause License

Copyright (c) 2018, Adrián
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/


//
// Created by Adrián on 21/7/22.
//

#ifndef RING_GAO_HPP
#define RING_GAO_HPP

#include <ring.hpp>
#include <unordered_map>
#include <vector>

namespace ring {

    namespace gao {

        template<class var_t = uint8_t >
        class gao_size {

        public:
            typedef var_t var_type;
            typedef uint64_t size_type;
            typedef struct {
                size_type n_triples;
                size_type weight;
            } info_var_type;

        private:
            const std::vector<triple_pattern>* m_ptr_triple_patterns;
            ring* m_ptr_ring;
            std::vector<std::pair<var_type, info_var_type>> m_gao;

            //TODO: refacer esto para que evite os forward steps
            uint64_t get_size_interval(const triple_pattern &triple_pattern) {
                if (triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
                    bwt_interval open_interval = m_ptr_ring->open_SPO();
                    return open_interval.size();
                } else if (triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
                    bwt_interval open_interval = m_ptr_ring->open_PSO();
                    auto cur_p = m_ptr_ring->next_P(open_interval, triple_pattern.term_p.value);
                    if (cur_p == 0 || cur_p != triple_pattern.term_p.value) {
                        return 0;
                    } else{
                        bwt_interval i_p = m_ptr_ring->down_P(cur_p);
                        return i_p.size();
                    }
                } else if (triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
                    bwt_interval open_interval = m_ptr_ring->open_OSP();
                    auto cur_o = m_ptr_ring->next_O(open_interval, triple_pattern.term_o.value);
                    if (cur_o == 0 || cur_o != triple_pattern.term_o.value) {
                        return 0;
                    } else{
                        bwt_interval i_s = m_ptr_ring->down_O(cur_o);
                        return i_s.size();
                    }
                } else if (!triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
                    bwt_interval open_interval = m_ptr_ring->open_SPO();
                    auto cur_s = m_ptr_ring->next_S(open_interval, triple_pattern.term_s.value);
                    if (cur_s == 0 || cur_s != triple_pattern.term_s.value) {
                        return 0;
                    } else{
                        bwt_interval i_s = m_ptr_ring->down_S(cur_s);
                        return i_s.size();
                    }
                } else if (!triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
                    bwt_interval open_interval = m_ptr_ring->open_SPO();
                    auto cur_s = m_ptr_ring->next_S(open_interval, triple_pattern.term_s.value);
                    if (cur_s == 0 || cur_s != triple_pattern.term_s.value) {
                        return 0;
                    } else{
                        bwt_interval i_s = m_ptr_ring->down_S(cur_s);
                        auto cur_p = m_ptr_ring->next_P_in_S(i_s, cur_s, triple_pattern.term_p.value);
                        if (cur_p == 0 || cur_p != triple_pattern.term_p.value) {
                            return 0;
                        }
                        bwt_interval i_p = m_ptr_ring->down_S_P(i_s, cur_s, cur_p);
                        return i_p.size();
                    }
                } else if (!triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
                    bwt_interval open_interval = m_ptr_ring->open_SOP();
                    auto cur_s = m_ptr_ring->next_S(open_interval, triple_pattern.term_s.value);
                    if (cur_s == 0 || cur_s != triple_pattern.term_s.value) {
                        return 0;
                    } else{
                        bwt_interval i_s = m_ptr_ring->down_S(cur_s);
                        auto cur_o = m_ptr_ring->next_O_in_S(i_s, triple_pattern.term_o.value);
                        if (cur_o == 0 || cur_o != triple_pattern.term_o.value) {
                            return 0;
                        }
                        bwt_interval i_o = m_ptr_ring->down_S_O(i_s, cur_o);
                        return i_o.size();
                    }
                } else if (triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
                    bwt_interval open_interval = m_ptr_ring->open_POS();
                    auto cur_p = m_ptr_ring->next_P(open_interval, triple_pattern.term_p.value);
                    if (cur_p == 0 || cur_p != triple_pattern.term_p.value) {
                        return 0;
                    } else{
                        bwt_interval i_p = m_ptr_ring->down_P(cur_p);
                        auto cur_o = m_ptr_ring->next_O_in_P(i_p, cur_p, triple_pattern.term_o.value);
                        if (cur_o == 0 || cur_o != triple_pattern.term_o.value) {
                            return 0;
                        }
                        bwt_interval i_o = m_ptr_ring->down_P_O(i_p, cur_p, cur_o);
                        return i_o.size();
                    }
                }
                return 0;
            }

            void var_to_map(const var_type var, const size_type size,
                            std::unordered_map<var_type, info_var_type> &hash_table){
                auto it = hash_table.find(var);
                if(it == hash_table.end()){
                    hash_table.insert(var, info_var_type{1, size});
                }else{
                    info_var_type& info = it->second;
                    ++info.n_triples;
                    if(info.weight > size){
                        info.weight = size;
                    }
                }
            }

            bool compare(const std::pair<var_type, info_var_type>& lhs,
                         const std::pair<var_type, info_var_type>& rhs)
            {
                info_var_type& linfo = lhs.second;
                info_var_type& rinfo = rhs.second;
                if(linfo.n_triples > 1 && rinfo.n_triples == 1){
                    return true;
                }
                if(linfo.n_triples == 1 && rinfo.n_triples > 1){
                    return false;
                }
                return linfo.weight < rinfo.weight;
            }

        public:

            gao_size(const std::vector<triple_pattern>* triple_patterns, ring* r){
                m_ptr_triple_patterns = triple_patterns;
                m_ptr_ring = r;

                std::unordered_map<var_type, info_var_type> hash_table;
                for (const triple_pattern& triple_pattern : *m_ptr_triple_patterns) { //TODO: esto está ben (solo refactoring)
                    size_type size = get_size_interval(triple_pattern, m_ptr_ring);
                    if(triple_pattern.s_is_variable()){
                        auto var = (var_type) triple_pattern.term_s.value;
                        var_to_map(var, size,hash_table);
                    }
                    if(triple_pattern.p_is_variable()){
                        auto var = (var_type) triple_pattern.term_p.value;
                        var_to_map(var, size, hash_table);
                    }
                    if(triple_pattern.o_is_variable()){
                        auto var = (var_type) triple_pattern.term_o.value;
                        var_to_map(var, size, hash_table);
                    }
                }

                m_gao.reserve(hash_table.size());
                for(auto& d : hash_table){
                    m_gao.push_back(d);
                }
                std::sort(m_gao.begin(), m_gao.end(), compare());

            }





        };

    }
}

#endif //RING_GAO_HPP
