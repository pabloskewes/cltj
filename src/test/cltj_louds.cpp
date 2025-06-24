//
// Created by adrian on 10/3/25.
//

#include <chrono>
#include <index/cltj_index_spo_lite.hpp>
#include <iostream>
#include <query/ltj_algorithm.hpp>
#include <triple_pattern.hpp>
#include <util/file_util.hpp>
#include <util/time_util.hpp>
#include <utility>

int main(int argc, char *argv[]) {
  // typedef ring::c_ring ring_type;
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <index> <spo-file>" << std::endl;
    return 0;
  }

  std::string index = argv[1];
  std::string spo_file = argv[2];
  cltj::compact_ltj graph;
  sdsl::load_from_file(graph, index);

  const sdsl::bit_vector &bv = graph.tries[0].bv;
  std::ofstream ofs(spo_file);
  sdsl::write_member(bv.size(), ofs);
  bv.write_data(ofs);
  ofs.close();
}