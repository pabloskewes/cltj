//
// Created by adrian on 19/12/24.
//

#include <cds/dyn_louds.hpp>

int main() {

  dyn_cds::dyn_louds seq(32);

  for (auto i = 0; i < 15000000; ++i) {
    auto p = 0;
    if (i > 0)
      p = rand() % i;
    seq.insert(p, 1, i, true);
    if (!seq.check()) {
      std::cout << "Error in sequence after inserting " << i << std::endl;
      exit(0);
    }
  }
}