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
// Created by Adrián on 6/9/22.
//

#ifndef RING_MUTHU_HPP
#define RING_MUTHU_HPP

namespace ring_ltj {

    class muthu {

    public:
        typedef uint64_t value_type;
        typedef uint64_t size_type;
        typedef sdsl::wm_int<bit_vector, bit_vector::rank_1_type,
                            sdsl::select_support_scan<1>, sdsl::select_support_scan<0>> wt_type;

    private:
        wt_type m_wt;

    public:
        muthu() = default;

        template<class Container>
        muthu(Container &c) {
            std::unordered_map<value_type, size_type> hash_table;
            int_vector<> pred(c.size());
            pred[0] = 0;
            for(size_type j = 1; j < c.size(); ++j){
                auto ht_it = hash_table.find(c[j]);
                if(ht_it == hash_table.end()){
                    pred[j] = 0;
                    hash_table.insert({c[j], j});
                }else{
                    pred[j] = ht_it->second;
                    ht_it->second = j;
                }
            }
            construct_im(m_wt, pred);
        }

        size_type count_distinct(const size_type l, const size_type r){
            return m_wt.count_range_search_2d(l, r, 0, l-1);
        }

        //! Copy constructor
        muthu(const muthu &o) {
            m_wt = o.m_wt;
        }

        //! Move constructor
        muthu(muthu &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        muthu &operator=(const muthu &o) {
            if (this != &o) {
                m_wt = o.m_wt;
            }
            return *this;
        }

        //! Move Operator=
        muthu &operator=(muthu &&o) {
            if (this != &o) {
                m_wt = std::move(o.m_wt);
            }
            return *this;
        }

        void swap(muthu &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_wt, o.m_wt);
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_wt.serialize(out, child, "wt");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_wt.load(in);
        }

    };
}

#endif //RING_MUTHU_HPP
