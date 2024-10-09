#ifndef CLTJ_CONFIG_H
#define CLTJ_CONFIG_H

#include <iostream>
#include <array>

namespace cltj{
    using namespace std;
/*
    Current Iterator should be set to the iterator that wants to be used
    The class that is assigned to CurrentIterator must implement the Iterator abstract class 
*/
//typedef CompactTrieIterator CurrentIterator;
//typedef CompactTrie CTrie;

    typedef std::array<uint32_t, 3> spo_triple;
    typedef uint8_t spo_order_type[3];
    typedef spo_order_type spo_orders_type[6];
    const static spo_orders_type spo_orders = {{0, 1, 2}, //SPO
                                              {0, 2, 1}, //SOP
                                              {1, 2, 0}, //POS
                                              {1, 0, 2}, //PSO
                                              {2, 0, 1}, //OSP
                                              {2, 1, 0}}; //OPS

    struct comparator_order {
        uint64_t i;
        comparator_order(uint64_t pi) {
            i = pi;
        };
        inline bool operator()(const spo_triple& t1, const spo_triple& t2) {
            if(t1[spo_orders[i][0]] == t2[spo_orders[i][0]]) {
                if(t1[spo_orders[i][1]] == t2[spo_orders[i][1]]) {
                    return t1[spo_orders[i][2]] < t2[spo_orders[i][2]];
                }
                return t1[spo_orders[i][1]] < t2[spo_orders[i][1]];
            }
            return t1[spo_orders[i][0]] < t2[spo_orders[i][0]];
        }
    };




}

#endif
