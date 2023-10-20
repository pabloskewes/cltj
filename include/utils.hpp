/*
 * utils.hpp
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


#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>
#include <configuration.hpp>
//#include <ltj_iterator.hpp>
#include <ltj_iterator_v2.hpp>

namespace ltj {

    namespace util {

        /*template<class Iterator, class Ring>
        uint64_t get_size_interval(Ring* ptr_ring, const Iterator &iter) {
            if(iter.cur_s == -1 && iter.cur_p == -1 && iter.cur_o == -1){
                return iter.i_s.size(); //open
            } else if (iter.cur_s == -1 && iter.cur_p != -1 && iter.cur_o == -1) {
                return iter.i_s.size(); //i_s = i_o
            } else if (iter.cur_s == -1 && iter.cur_p == -1 && iter.cur_o != -1) {
                return iter.i_s.size(); //i_s = i_p
            } else if (iter.cur_s != -1 && iter.cur_p == -1 && iter.cur_o == -1) {
                return iter.i_o.size(); //i_o = i_p
            } else if (iter.cur_s != -1 && iter.cur_p != -1 && iter.cur_o == -1) {
                return iter.i_o.size();
            } else if (iter.cur_s != -1 && iter.cur_p == -1 && iter.cur_o != -1) {
                return iter.i_p.size();
            } else if (iter.cur_s == -1 && iter.cur_p != -1 && iter.cur_o != -1) {
                return iter.i_s.size();
            }
            return 0;
        }*/


        struct trait_size_old {

            template<class Iterator, class Ring>
            static uint64_t subject(Ring *ptr_ring, Iterator &iter) {
                return iter.i_s.size();
            }

            template<class Iterator, class Ring>
            static uint64_t predicate(Ring *ptr_ring, Iterator &iter) {
                return iter.i_p.size();
            }

            template<class Iterator, class Ring>
            static uint64_t object(Ring *ptr_ring, Iterator &iter) {
                return iter.i_o.size();
            }

        };

        struct trait_size {

            template<class Iterator>
            static uint64_t subject(Iterator &iter) {
                if(iter.nfixed==0) return -1ULL -1;
                if(iter.nfixed==1){
                    return iter.subtree_size_fixed1(s);
                }else{
                    return iter.subtree_size_fixed2();
                }
            }

            template<class Iterator>
            static uint64_t predicate(Iterator &iter) {
                if(iter.nfixed==0) return -1ULL -1;
                if(iter.nfixed==1){
                    return iter.subtree_size_fixed1(p);
                }else{
                    return iter.subtree_size_fixed2();
                }
            }

            template<class Iterator>
            static uint64_t object(Iterator &iter) {
                if(iter.nfixed==0) return -1ULL -1;
                if(iter.nfixed==1){
                    return iter.subtree_size_fixed1(o);
                }else{
                    return iter.subtree_size_fixed2();
                }
            }
        };

        struct trait_distinct {

            template<class Iterator>
            static uint64_t subject(Iterator &iter) {
                return iter.children(s);
            }

            template<class Iterator>
            static uint64_t predicate(Iterator &iter) {
                return iter.children(p);
            }

            template<class Iterator>
            static uint64_t object(Iterator &iter) {
                return iter.children(o);
            }
        };
    }

}



#endif