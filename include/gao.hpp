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
#include <utils.hpp>
#include <unordered_set>

namespace ring {

    namespace gao {

        template<class var_t = uint8_t >
        class gao_size {

        public:
            typedef var_t var_type;
            typedef uint64_t size_type;
            typedef struct {
                var_type name;
                size_type weight;
                size_type n_triples;
                std::unordered_set<var_type> related;
            } info_var_type;

            typedef std::pair<size_type, var_type> pair_type;
            typedef std::priority_queue<pair_type, std::vector<pair_type>, greater<pair_type>> min_heap_type;

        private:
            const std::vector<triple_pattern>* m_ptr_triple_patterns;
            ring* m_ptr_ring;
            std::vector<var_type> m_gao;


            void var_to_vector(const var_type var, const size_type size,
                               std::unordered_map<var_type, size_type> &hash_table,
                               std::vector<info_var_type> &vec){

                auto it = hash_table.find(var);
                if(it == hash_table.end()){
                    info_var_type info;
                    info.name = var;
                    info.weight = size;
                    info.n_triples = 1;
                    vec.emplace_back(info);
                    hash_table.insert({var, vec.size()-1});
                }else{
                    info_var_type& info = vec[it->second];
                    ++info.n_triples;
                    if(info.weight > size){
                        info.weight = size;
                    }
                }
            }

            void var_to_related(const var_type var, const var_type rel,
                                std::unordered_map<var_type, size_type> &hash_table,
                                std::vector<info_var_type> &vec){

                auto pos_var = hash_table[var];
                vec[pos_var].related.insert(rel);
                auto pos_rel = hash_table[rel];
                vec[pos_rel].related.insert(var);
            }

            void fill_heap(const var_type var,
                           std::unordered_map<var_type, size_type> &hash_table,
                           std::vector<info_var_type> &vec,
                           std::vector<bool> &checked,
                           min_heap_type &heap){

                auto pos_var = hash_table[var];
                for(const auto &e : vec[pos_var].related){
                    auto pos_rel = hash_table[e];
                    if(!checked[pos_rel]){
                        heap.push({vec[pos_rel].weight, e});
                        checked[pos_rel] = true;
                    }
                }
            }

            struct compare_var_info
            {
                inline bool operator() (const info_var_type& linfo, const info_var_type& rinfo)
                {
                    if(linfo.n_triples>1 && rinfo.n_triples==1){
                        return true;
                    }
                    if(linfo.n_triples==1 && rinfo.n_triples>1){
                        return false;
                    }
                    return linfo.weight < rinfo.weight;
                }
            };

        public:

            const std::vector<var_type> &gao = m_gao;


            gao_size(const std::vector<triple_pattern>* triple_patterns, ring* r){
                m_ptr_triple_patterns = triple_patterns;
                m_ptr_ring = r;


                //1. Filling var_info with data about each variable
                //std::cout << "Filling... " << std::flush;
                std::vector<info_var_type> var_info;
                std::unordered_map<var_type, size_type> hash_table_position;
                for (const triple_pattern& triple_pattern : *m_ptr_triple_patterns) {
                    size_type size = util::get_size_interval(triple_pattern, m_ptr_ring);
                    bool s = false, p = false, o = false;
                    var_type var_s, var_p, var_o;
                    if(triple_pattern.s_is_variable()){
                        s = true;
                        var_s = (var_type) triple_pattern.term_s.value;
                        var_to_vector(var_s, size,hash_table_position, var_info);
                    }
                    if(triple_pattern.p_is_variable()){
                        p = true;
                        var_p = (var_type) triple_pattern.term_p.value;
                        var_to_vector(var_p, size,hash_table_position, var_info);
                    }
                    if(triple_pattern.o_is_variable()){
                        o = true;
                        var_o = triple_pattern.term_o.value;
                        var_to_vector(var_o, size,hash_table_position, var_info);
                    }

                    if(s && p){
                        var_to_related(var_s, var_p, hash_table_position, var_info);
                    }
                    if(s && o){
                        var_to_related(var_s, var_o, hash_table_position, var_info);
                    }
                    if(p && o){
                        var_to_related(var_p, var_o, hash_table_position, var_info);
                    }
                }
                //std::cout << "Done. " << std::endl;

                //2. Sorting variables according to their weights.
                //std::cout << "Sorting... " << std::flush;
                std::sort(var_info.begin(), var_info.end(), compare_var_info());
                for(size_type i = 0; i < var_info.size(); ++i){
                    hash_table_position[var_info[i].name] = i;
                }
                //std::cout << "Done. " << std::endl;

                //3. Choosing the variables
                size_type i = 0;
                //std::cout << "Choosing GAO ... " << std::flush;
                std::vector<bool> checked(var_info.size(), false);
                m_gao.reserve(var_info.size());
                while(i < var_info.size()){
                    if(!checked[i]){
                        m_gao.push_back(var_info[i].name); //Adding var to gao
                        checked[i] = true;
                        if(var_info[i].related.size() > 1){
                            min_heap_type heap; //Stores the variables that are related with the chosen ones
                            auto var_name = var_info[i].name;
                            fill_heap(var_name, hash_table_position, var_info, checked,heap);
                            while(!heap.empty()){
                                var_name = heap.top().second;
                                heap.pop();
                                m_gao.push_back(var_name);
                                fill_heap(var_name, hash_table_position, var_info, checked, heap);
                            }
                        }
                    }
                    ++i;
                }
                //std::cout << "Done. " << std::endl;
            }

        };

    }
}

#endif //RING_GAO_HPP
