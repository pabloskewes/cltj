#include <cassert>
#include <cltj/hashing/mphf_utils.hpp>
#include <iostream>

using namespace cltj::hashing;

void test_mod_mul() {
  std::cout << "Testing mod_mul...\n";

  // Small numbers
  assert(mod_mul(3, 4, 5) == 2);  // 3*4=12 ≡ 2 mod 5
  assert(mod_mul(7, 8, 11) == 1); // 7*8=56 ≡ 1 mod 11

  // Large numbers
  uint64_t large = 1ULL << 60;
  assert(
      mod_mul(large, large + 1, large + 2) == 1
  ); // (2^60)*(2^60+1) mod (2^60+2)

  // Edge cases
  assert(mod_mul(0, 100, 7) == 0);
  assert(mod_mul(100, 0, 7) == 0);
  assert(mod_mul(1, 1, 2) == 1);

  std::cout << "mod_mul tests passed!\n";
}

void test_mod_pow() {
  std::cout << "Testing mod_pow...\n";

  // Small numbers
  assert(mod_pow(2, 3, 5) == 3); // 2^3=8 ≡ 3 mod 5
  assert(mod_pow(3, 4, 7) == 4); // 3^4=81 ≡ 4 mod 7

  // Large exponents
  assert(mod_pow(2, 60, 1000) == 376); // 2^60 mod 1000

  // Edge cases
  assert(mod_pow(0, 100, 7) == 0);
  assert(mod_pow(100, 0, 7) == 1);
  assert(mod_pow(1, 100, 2) == 1);

  std::cout << "mod_pow tests passed!\n";
}

void test_is_prime() {
  std::cout << "Testing is_prime...\n";

  // Known primes
  assert(is_prime(2));
  assert(is_prime(3));
  assert(is_prime(13));
  assert(is_prime(1'000'000'007));

  // Known composites
  assert(!is_prime(1));
  assert(!is_prime(4));
  assert(!is_prime(15));
  assert(!is_prime(1'000'000'008));

  // Edge cases
  assert(!is_prime(0));
  assert(!is_prime(1));

  std::cout << "is_prime tests passed!\n";
}

void test_next_prime() {
  std::cout << "Testing next_prime...\n";

  assert(next_prime(1) == 2);
  assert(next_prime(2) == 2);
  assert(next_prime(3) == 3);
  assert(next_prime(4) == 5);
  assert(next_prime(100) == 101);
  assert(next_prime(1'000'000) == 1'000'003);

  // Verify it skips even numbers
  assert(next_prime(1'000'000'000) == 1'000'000'007);

  std::cout << "next_prime tests passed!\n";
}

void test_all_coprime() {
  std::cout << "Testing all_coprime...\n";

  std::vector<uint64_t> keys1 = {1, 2, 3, 4};
  assert(all_coprime(keys1, 5) == true);

  std::vector<uint64_t> keys2 = {5, 10, 15};
  assert(all_coprime(keys2, 5) == false);

  // Edge cases
  std::vector<uint64_t> empty;
  assert(all_coprime(empty, 5) == true);

  std::vector<uint64_t> zero = {0};
  assert(all_coprime(zero, 5) == true); // 0 is coprime to everything

  std::cout << "all_coprime tests passed!\n";
}

int main() {
  test_mod_mul();
  test_mod_pow();
  test_is_prime();
  test_next_prime();
  test_all_coprime();

  std::cout << "All mphf_utils tests passed successfully!\n";
  return 0;
}