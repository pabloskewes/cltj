/*
 * gao.hpp
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


#ifndef RING_GAO_ADAPTIVE_HPP
#define RING_GAO_ADAPTIVE_HPP

#include <ring.hpp>
#include <ltj_iterator.hpp>
#include <ltj_iterator_unidirectional.hpp>
#include <triple_pattern.hpp>
#include <unordered_map>
#include <vector>
#include <utils.hpp>
#include <unordered_set>

namespace ring {

    namespace gao {


        template<class ltj_iterator_t = ltj_iterator <ring<>, uint8_t, uint64_t>,
                class gao_trait_t = util::trait_size>
        class gao_adaptive {

        public:
            typedef ltj_iterator_t ltj_iter_type;
            typedef typename ltj_iter_type::var_type var_type;
            typedef uint64_t size_type;
            typedef typename ltj_iter_type::ring_type ring_type;


            /*enum spo_type {subject, predicate, object};
            typedef struct {
                std::vector<spo_type> vec_spo;
                size_type iter_pos;
            } spo_iter_type;*/

            typedef struct {
                var_type name;
                size_type weight;
                size_type pos;
                //std::vector<spo_iter_type> iterators;
                std::unordered_set<var_type> related;
                bool is_bound;
            } info_var_type;

            typedef  std::list<size_type> list_type;
            typedef  typename list_type::iterator list_iterator_type;

            typedef struct {
                size_type pos;
                size_type w;
            } update_type;


            typedef std::vector<update_type> version_type;

            typedef std::pair<size_type, var_type> pair_type;
            typedef gao_trait_t gao_trait_type;
            typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
            typedef std::stack<version_type> versions_type;
            typedef std::stack<size_type> bound_type;

        private:
            const std::vector<triple_pattern> *m_ptr_triple_patterns;
            const std::vector<ltj_iter_type> *m_ptr_iterators;
            const var_to_iterators_type *m_ptr_var_iterators;
            ring_type *m_ptr_ring;
            std::unordered_map<var_type, size_type> m_hash_table_position;
            std::vector<info_var_type> m_var_info;
            std::vector<var_type> m_lonely;
            size_type m_index;
            list_type m_not_bound;
            bound_type m_bound;
            versions_type m_versions;



            void copy(const gao_adaptive &o) {
                m_ptr_triple_patterns = o.m_ptr_triple_patterns;
                m_ptr_iterators = o.m_ptr_iterators;
                m_ptr_var_iterators = o.m_ptr_var_iterators;
                m_ptr_ring = o.m_ptr_ring;
                m_hash_table_position = o.m_hash_table_position;
                m_not_bound = o.m_not_bound;
                m_lonely = o.m_lonely;
                m_index = o.m_index;
                m_bound = o.m_bound;
                m_versions = o.m_versions;
                m_var_info = o.m_var_info;
            }


            bool var_to_vector(const var_type var, const size_type i, const state_type state) {

                const auto &iters = m_ptr_var_iterators->at(var);
                if(iters.size() == 1){
                    m_lonely.emplace_back(var);
                    return false;
                }else{
                    auto it = m_hash_table_position.find(var);
                    if (it == m_hash_table_position.end()) {
                        info_var_type info;
                        info.name = var;
                        info.weight = gao_trait_type::get(m_ptr_ring, m_ptr_iterators->at(i), state);
                        info.is_bound = false;
                        info.pos = m_var_info.size();
                        m_var_info.emplace_back(info);
                        m_hash_table_position.insert({var, info.pos});
                        m_not_bound.push_back(info.pos);
                    } else {
                        auto size = gao_trait_type::get(m_ptr_ring, m_ptr_iterators->at(i), state);
                        info_var_type &info = m_var_info[it->second];
                        if (info.weight > size) {
                            info.weight = size;
                        }
                    }
                    return true;
                }

            }

            void var_to_related(const var_type var, const var_type rel) {


                auto pos_var = m_hash_table_position[var];
                m_var_info[pos_var].related.insert(rel);
                auto pos_rel = m_hash_table_position[rel];
                m_var_info[pos_rel].related.insert(var);
            }


        public:

            gao_adaptive() = default;

            gao_adaptive(const std::vector<triple_pattern> *triple_patterns,
                       const std::vector<ltj_iter_type> *iterators,
                       const var_to_iterators_type *var_iterators,
                       ring_type *r) {
                m_ptr_triple_patterns = triple_patterns;
                m_ptr_iterators = iterators;
                m_ptr_var_iterators = var_iterators;
                m_ptr_ring = r;


                //1. Filling var_info with data about each variable
                //std::cout << "Filling... " << std::flush;
                size_type i = 0;
                for (const triple_pattern &triple_pattern : *m_ptr_triple_patterns) {
                    bool s = false, p = false, o = false;
                    var_type var_s, var_p, var_o;
                    if (triple_pattern.s_is_variable()) {
                        var_s = (var_type) triple_pattern.term_s.value;
                        //size = gao_trait_type::subject(m_ptr_ring, m_ptr_iterators->at(i));
                        s = var_to_vector(var_s, i, state_type::s);
                    }
                    if (triple_pattern.p_is_variable()) {
                        var_p = (var_type) triple_pattern.term_p.value;
                        //size = gao_trait_type::predicate(m_ptr_ring, m_ptr_iterators->at(i));
                        p = var_to_vector(var_p, i, state_type::p);
                    }
                    if (triple_pattern.o_is_variable()) {
                        var_o = (var_type) triple_pattern.term_o.value;
                        //size = gao_trait_type::object(m_ptr_ring, m_ptr_iterators->at(i));
                        o = var_to_vector(var_o, i, state_type::o);
                    }

                    if (s && p) {
                        var_to_related(var_s, var_p);
                    }
                    if (s && o) {
                        var_to_related(var_s, var_o);
                    }
                    if (p && o) {
                        var_to_related(var_p, var_o);
                    }
                    ++i;
                }
                m_index = 0;
                //std::cout << "Done. " << std::endl;

                //std::cout << "Done. " << std::endl;
            }

            //! Copy constructor
            gao_adaptive(const gao_adaptive &o) {
                copy(o);
            }

            //! Move constructor
            gao_adaptive(gao_adaptive &&o) {
                *this = std::move(o);
            }

            //! Copy Operator=
            gao_adaptive &operator=(const gao_adaptive &o) {
                if (this != &o) {
                    copy(o);
                }
                return *this;
            }

            //! Move Operator=
            gao_adaptive &operator=(gao_adaptive &&o) {
                if (this != &o) {
                    m_ptr_triple_patterns = std::move(o.m_ptr_triple_patterns);
                    m_ptr_iterators = std::move(o.m_ptr_iterators);
                    m_ptr_ring = std::move(o.m_ptr_ring);
                    m_ptr_var_iterators = o.m_ptr_var_iterators;
                    m_hash_table_position = std::move(o.m_hash_table_position);
                    m_lonely = std::move(o.m_lonely);
                    m_index = o.m_index;
                    m_bound = std::move(o.m_bound);
                    m_versions = std::move(o.m_versions);
                    m_var_info = std::move(o.m_var_info);
                    m_not_bound = std::move(o.m_not_bound);
                }
                return *this;
            }

            void swap(gao_adaptive &o) {
                std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
                std::swap(m_ptr_iterators, o.m_ptr_iterators);
                std::swap(m_ptr_var_iterators, o.m_ptr_var_iterators);
                std::swap(m_ptr_ring, o.m_ptr_ring);
                std::swap(m_hash_table_position, o.m_hash_table_position);
                std::swap(m_lonely, o.m_lonely);
                std::swap(m_index, o.m_index);
                std::swap(m_bound, o.m_bound);
                std::swap(m_versions, o.m_versions);
                std::swap(m_var_info, o.m_var_info);
                std::swap(m_not_bound, o.m_not_bound);
            }

            inline var_type next() {

                if(m_index < m_var_info.size()){ //No lonely
                    size_type min = -1ULL;
                    list_iterator_type min_iter;
                    // Lineal search on variables that are not is_bound
                    for(auto iter = m_not_bound.begin(); iter != m_not_bound.end(); ++iter){
                        //Take the one with the smallest weight
                        const auto &v = m_var_info[*iter];
                        if(min > v.weight){
                            min = v.weight;
                            min_iter = iter;
                        }
                    }
                    //Remove from list
                    size_type min_pos = *min_iter;
                    m_var_info[min_pos].is_bound = true;
                    m_bound.emplace(min_pos);
                    m_not_bound.erase(min_iter); //Remove the variable from a list of unbound variables
                    ++m_index;
                    return  m_var_info[min_pos].name;
                }else{
                    //Return the next lonely variable
                    ++m_index;
                    return m_lonely[m_index-m_var_info.size()-1];
                }

            }



            inline void down() {

                if(m_index-1 < m_var_info.size()){ //No lonely

                    auto pos_last = m_bound.top();
                    const auto &related = m_var_info[pos_last].related;
                    version_type version;
                    for(const auto &rel : related){ //Iterates on the related variables
                        const auto pos = m_hash_table_position[rel];
                        if(!m_var_info[pos].is_bound){
                            bool u = false;
                            size_type min_w = m_var_info[pos].weight, w;
                            auto &iters = m_ptr_var_iterators->at(rel);
                            for(ltj_iter_type* iter : iters){ //Check each iterator
                                if(iter->is_variable_subject(rel)){
                                    w = gao_trait_type::subject(m_ptr_ring, *iter); //New weight
                                }else if (iter->is_variable_predicate(rel)){
                                    w = gao_trait_type::predicate(m_ptr_ring, *iter);
                                }else{
                                    w = gao_trait_type::object(m_ptr_ring, *iter);
                                }
                                if(min_w > w) {
                                    min_w = w;
                                    u = true;
                                }
                            }

                            if(u){
                                update_type update{pos,  m_var_info[pos].weight};
                                version.emplace_back(update);  //Store an update
                                m_var_info[pos].weight = min_w;
                            }
                        }
                    }
                    m_versions.emplace(version); //Add the updates to versions
                }

            };

            inline void up() {
                if(m_index-1 < m_var_info.size()){ //No lonely
                    for(const update_type& update : m_versions.top()){ //Restart the weights
                        m_var_info[update.pos].weight = update.w;
                    }
                    m_versions.pop();
                }

            };

            inline void done() {
                --m_index;
                if(m_index < m_var_info.size()) { //No lonely
                    auto pos = m_bound.top();
                    m_not_bound.push_back(pos); //Restart the list of unbound variables
                    m_var_info[pos].is_bound = false;
                    m_bound.pop();
                }
            }


            inline size_type size() {
                return m_var_info.size() + m_lonely.size();
            }
        };
    };
}

#endif //RING_GAO_SIZE_HPP
