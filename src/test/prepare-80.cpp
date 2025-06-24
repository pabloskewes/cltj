//
// Created by adrian on 10/12/24.
//

#include <algorithm>
#include <cltj_config.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << argv[0] << " <dataset>" << std::endl;
    return 0;
  }

  std::string dataset = argv[1];

  std::vector<cltj::spo_triple> D;

  std::ifstream ifs(dataset);
  uint32_t s, p, o;
  uint64_t k = 0;
  cltj::spo_triple spo;
  do {
    ifs >> s >> p >> o;
    spo[0] = s;
    spo[1] = p;
    spo[2] = o;
    D.emplace_back(spo);
    ++k;

  } while (!ifs.eof());

  D.shrink_to_fit();
  std::cout << "Triples: " << D.size() << "." << std::endl;
  std::cout << "Dataset: " << 3 * D.size() * sizeof(::uint32_t) << " bytes."
            << std::endl;
  // Shuffle the dataset
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(D.begin(), D.end(), g);

  uint64_t num_build = D.size() * 0.8; // num triples in the build phase
  // uint64_t num_build = D.size();

  std::string f_data = dataset + ".80";
  std::ofstream of_dataset(f_data);
  for (uint64_t i = 0; i < num_build; ++i) {
    of_dataset << D[i][0] << " " << D[i][1] << " " << D[i][2] << std::endl;
  }
  of_dataset.close();

  std::string f_updates = dataset + ".updates";
  std::ofstream of_updates(f_updates);
  uint64_t i_del = 0;
  uint64_t i_ins = num_build;
  std::uniform_int_distribution<uint> dist1(0, 1);
  uint64_t updates =
      1500000; // 1295*1000 (creo mas por si en el futuro se necesitan)
  for (uint64_t i = 0; i < updates; ++i) {
    bool ins = dist1(rd);
    if (ins) {
      of_updates << "Insert" << std::endl;
      of_updates << D[i_ins][0] << " " << D[i_ins][1] << " " << D[i_ins][2]
                 << std::endl;
      ++i_ins;
      if (i_ins == D.size())
        i_ins = 0;
    } else {
      of_updates << "Delete" << std::endl;
      of_updates << D[i_del][0] << " " << D[i_del][1] << " " << D[i_del][2]
                 << std::endl;
      ++i_del;
      if (i_del == D.size())
        i_del = 0;
    }
  }
  of_updates.close();
}
