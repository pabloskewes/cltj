#ifndef CLTJ_CONFIG_H
#define CLTJ_CONFIG_H

#include <array>
#include <cstdint>
#include <iostream>

enum state_type { s = 0, p = 1, o = 2 };
namespace cltj {
using namespace std;
/*
    Current Iterator should be set to the iterator that wants to be used
    The class that is assigned to CurrentIterator must implement the Iterator
   abstract class
*/
// typedef CompactTrieIterator CurrentIterator;
// typedef CompactTrie CTrie;
typedef std::array<uint32_t, 3> spo_triple;
typedef std::array<std::string, 3> user_triple;
typedef uint8_t spo_order_type[3];
typedef spo_order_type spo_orders_type[6];
const static spo_orders_type spo_orders = {
    {0, 1, 2}, // SPO
    {0, 2, 1}, // SOP
    {1, 2, 0}, // POS
    {1, 0, 2}, // PSO
    {2, 0, 1}, // OSP
    {2, 1, 0}
}; // OPS

// given an index i of a partial trie, we can compute the switching as
// ts_full_map[i/2]
const static std::array<uint8_t, 3> ts_full_map = {4, 0, 2};
// given an index i of a full trie, we can compute the switching as
// ts_part_map[i/2]
const static std::array<uint8_t, 3> ts_part_map = {3, 5, 1};

struct comparator_order {
  uint64_t i;
  comparator_order(uint64_t pi) {
    i = pi;
  };
  inline bool operator()(const spo_triple &t1, const spo_triple &t2) {
    if (t1[spo_orders[i][0]] == t2[spo_orders[i][0]]) {
      if (t1[spo_orders[i][1]] == t2[spo_orders[i][1]]) {
        return t1[spo_orders[i][2]] < t2[spo_orders[i][2]];
      }
      return t1[spo_orders[i][1]] < t2[spo_orders[i][1]];
    }
    return t1[spo_orders[i][0]] < t2[spo_orders[i][0]];
  }
};

} // namespace cltj

#endif
