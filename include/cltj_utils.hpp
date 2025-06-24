/*
 * cltj_utils.hpp
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


#ifndef CLTJ_UTILS_H
#define CLTJ_UTILS_H

#include <iostream>
namespace ltj {

    namespace util {


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