#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>
#include "ring.hpp"


namespace ring {

    namespace util {


        template<class Iterator>
        uint64_t get_size_interval(const Iterator &iter) {
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
        }
    }

}



#endif 
