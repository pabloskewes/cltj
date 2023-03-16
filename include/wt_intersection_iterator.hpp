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
// Created by Adrián on 22/9/22.
//

#ifndef RING_RPQ_WT_INTERSECTION_ITERATOR_HPP
#define RING_RPQ_WT_INTERSECTION_ITERATOR_HPP

#include <algorithm>
#include <utility>
#include <sdsl/wt_helper.hpp>

namespace sdsl {


    template<class wt_t>
    class wt_intersection_iterator {
    public:
        typedef wt_t wt_type;
        typedef typename wt_type::size_type size_type;
        typedef typename wt_type::value_type value_type;
        typedef typename wt_type::node_type node_type;
        typedef std::vector<range_type> range_vec_type;
        typedef std::vector<node_type> node_vec_type;
        typedef std::pair<node_vec_type, range_vec_type> pnvr_type;
        typedef std::stack<pnvr_type> stack_type;

    private:
        std::vector<const wt_type*> m_wt_ptrs;
        stack_type m_stack;
        size_type m_size = 0;

        void copy(const wt_intersection_iterator &o) {
            m_wt_ptrs = o.m_wt_ptrs;
            m_stack = o.m_stack;
            m_size = o.m_size;
        }

    public:

        wt_intersection_iterator() = default;

        template<class Iterator>
        wt_intersection_iterator(const std::vector<Iterator*>& iterators, const uint64_t x_j){
            m_size = iterators.size();
            pnvr_type element;
            for(size_type i = 0; i < m_size; ++i){
                auto wm_data = iterators[i]->get_wm_data(x_j);
                m_wt_ptrs.emplace_back(wm_data.wm_ptr);
                element.first.emplace_back(std::move(m_wt_ptrs[i]->root()));
                element.second.emplace_back(std::move(wm_data.range));
            }
            m_stack.emplace(element);
        }

        /***
         * Next value of an intersection between WTs on the same alphabet
         */
        value_type next(){

            while (!m_stack.empty()) {
                const pnvr_type &x = m_stack.top();
                if (m_wt_ptrs[0]->is_leaf(x.first[0])) {
                   auto r = value_type(x.first[0].sym);
                   m_stack.pop();
                   return r;
                }else{
                    node_vec_type left_nodes, right_nodes;
                    range_vec_type left_ranges, right_ranges;
                    std::array<range_type, 2> child_ranges;
                    size_type rnk;
                    for(size_type i = 0; i < m_size; ++i){
                        auto child =  m_wt_ptrs[i]->my_expand(x.first[i], x.second[i],
                                                       child_ranges[0], child_ranges[1], rnk);

                        if(left_nodes.size() == i && !empty(child_ranges[0])){
                            left_nodes.emplace_back(std::move(child[0]));
                            left_ranges.emplace_back(child_ranges[0]);
                        }

                        if(right_nodes.size() == i && !empty(child_ranges[1])){
                            right_nodes.emplace_back(std::move(child[1]));
                            right_ranges.emplace_back(child_ranges[1]);
                        }

                        if(right_nodes.size() < i+1 && left_nodes.size() < i+1){
                            break;
                        }
                    }
                    m_stack.pop();
                    if(right_nodes.size() == m_size){
                        m_stack.emplace(std::move(right_nodes), std::move(right_ranges));
                    }
                    if(left_nodes.size() == m_size){
                        m_stack.emplace(std::move(left_nodes), std::move(left_ranges));
                    }
                }
            }
            return 0; //No intersection
        }


        bool is_empty() const {
            return m_size == 0;
        }

        //! Copy constructor
        wt_intersection_iterator(const wt_intersection_iterator &o) {
            copy(o);
        }

        //! Move constructor
        wt_intersection_iterator(wt_intersection_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        wt_intersection_iterator &operator=(const wt_intersection_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        wt_intersection_iterator &operator=(wt_intersection_iterator &&o) {
            if (this != &o) {
                m_wt_ptrs = std::move(o.m_wt_ptrs);
                m_stack = std::move(o.m_stack);
                m_size = o.m_size;
            }
            return *this;
        }

        void swap(wt_intersection_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_wt_ptrs, o.m_wt_ptrs);
            std::swap(m_stack, o.m_stack);
            std::swap(m_size, o.m_size);
        }
    };

}

#endif //RING_RPQ_WT_INTERSECTION_ITERATOR_HPP
