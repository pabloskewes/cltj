// Minimal harness to exercise MPHF interface with the book's setup.
#include <include/hashing/mphf_bdz.hpp>
#include <iostream>
#include <vector>

using cltj::hashing::MPHF;

int main() {
  std::cout << "Hello MPHF (book example harness)\n";

  // Book example has 5 keys; use placeholders for now.
  std::vector<uint64_t> keys = {1, 2, 3, 4, 5};

  MPHF mphf;
  bool ok = mphf.build(keys);
  std::cout << "build(keys) -> " << (ok ? "success" : "failure") << "\n";

  for (auto k : keys) {
    uint32_t h = mphf.query(k);
    std::cout << "key=" << k << " -> h=" << h << "\n";
  }

  return 0;
}
