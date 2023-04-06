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


#ifndef RING_GAO_RANDOM_HPP
#define RING_GAO_RANDOM_HPP

#include <ring.hpp>
#include <ltj_iterator.hpp>
#include <ltj_iterator_unidirectional.hpp>
#include <triple_pattern.hpp>
#include <unordered_map>
#include <vector>
#include <utils.hpp>
#include <unordered_set>

namespace ring {

    namespace veo {


        template<class ltj_iterator_t = ltj_iterator <ring<>, uint8_t, uint64_t>,
                class veo_trait_t = util::trait_size>
        class veo_random {

        public:
            typedef ltj_iterator_t ltj_iter_type;
            typedef typename ltj_iter_type::var_type var_type;
            typedef uint64_t size_type;
            typedef typename ltj_iter_type::ring_type ring_type;

            typedef std::pair<size_type, var_type> pair_type;
            typedef veo_trait_t veo_trait_type;
            typedef std::priority_queue<pair_type, std::vector<pair_type>, greater<pair_type>> min_heap_type;
            typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;

        private:
            const std::vector<triple_pattern> *m_ptr_triple_patterns;
            const std::vector<ltj_iter_type> *m_ptr_iterators;
            ring_type *m_ptr_ring;
            std::vector<var_type> m_order;
            size_type m_index;

            void copy(const veo_random &o) {
                m_ptr_triple_patterns = o.m_ptr_triple_patterns;
                m_ptr_iterators = o.m_ptr_iterators;
                m_ptr_ring = o.m_ptr_ring;
                m_order = o.m_order;
                m_index = o.m_index;
            }


        public:

            veo_random() = default;

            veo_random(const std::vector<triple_pattern> *triple_patterns,
                       const std::vector<ltj_iter_type> *iterators,
                       const var_to_iterators_type *var_iterators,
                       ring_type *r) {
                m_ptr_triple_patterns = triple_patterns;
                m_ptr_iterators = iterators;
                m_ptr_ring = r;

                //1. Filling var_info with data about each variable
                //std::cout << "Filling... " << std::flush;
                std::unordered_set<var_type> added_vars;
                var_type var_s, var_p, var_o;
                for (const triple_pattern &triple_pattern : *m_ptr_triple_patterns) {
                    if (triple_pattern.s_is_variable()) {
                        var_s = (var_type) triple_pattern.term_s.value;
                        added_vars.insert(var_s);
                    }
                    if (triple_pattern.p_is_variable()) {
                        var_p = (var_type) triple_pattern.term_p.value;
                        added_vars.insert(var_p);
                    }
                    if (triple_pattern.o_is_variable()) {
                        var_o = (var_type) triple_pattern.term_o.value;
                        added_vars.insert(var_o);
                    }
                }
                //std::cout << "Done. " << std::endl;

                //2. Sorting variables according to their weights.
                //std::cout << "Sorting... " << std::flush;
                for(auto &v : added_vars){
                    m_order.emplace_back(v);
                }
                std::random_device rd;
                std::mt19937 g(rd());
                std::shuffle(m_order.begin(), m_order.end(), g);
                m_index = 0;
                //std::cout << "Done. " << std::endl;
            }

            //! Copy constructor
            veo_random(const veo_random &o) {
                copy(o);
            }

            //! Move constructor
            veo_random(veo_random &&o) {
                *this = std::move(o);
            }

            //! Copy Operator=
            veo_random &operator=(const veo_random &o) {
                if (this != &o) {
                    copy(o);
                }
                return *this;
            }

            //! Move Operator=
            veo_random &operator=(veo_random &&o) {
                if (this != &o) {
                    m_ptr_triple_patterns = std::move(o.m_ptr_triple_patterns);
                    m_ptr_iterators = std::move(o.m_ptr_iterators);
                    m_ptr_ring = o.m_ptr_ring;
                    m_order = std::move(o.m_order);
                    m_index = o.m_index;
                }
                return *this;
            }

            void swap(veo_random &o) {
                std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
                std::swap(m_ptr_iterators, o.m_ptr_iterators);
                std::swap(m_ptr_ring, o.m_ptr_ring);
                std::swap(m_order, o.m_order);
                std::swap(m_index, o.m_index);
            }

            inline var_type next() {
                ++m_index;
                return m_order[m_index-1];
            }



            inline void down() {
                //++m_index;
            };

            inline void up() {
               // --m_index;
            };

            inline void done() {
                --m_index;
            };

            inline size_type size() {
                return m_order.size();
            }

        };
    };
}

#endif //RING_veo_RANDOM_HPP
