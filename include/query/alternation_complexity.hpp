#ifndef ALTERNATION_COMPLEXITY_HPP
#define ALTERNATION_COMPLEXITY_HPP

#include <cstdint>
#include <iostream>
#include <vector>

namespace ltj {

/**
 * @brief Represents an interval in the alternation complexity calculation
 */
struct Interval {
  uint64_t left;
  uint64_t right;
  bool is_singleton;

  Interval(uint64_t l, uint64_t r, bool singleton = false)
      : left(l), right(r), is_singleton(singleton) {
  }

  // For debugging
  void print() const {
    if (is_singleton) {
      std::cout << "{" << left << "}";
    } else {
      std::cout << "[" << left << ", " << right << ")";
    }
  }
};

/**
 * @brief Calculate alternation complexity using iterators directly
 *
 * @param iterators Vector of iterators for the k-way intersection
 * @param variable The variable being processed
 * @return The alternation complexity (number of intervals in partition
 * certificate)
 */
template <class ltj_iter_type, class var_type>
int calculate_alternation_complexity(
    const std::vector<ltj_iter_type *> &iterators,
    var_type variable
) {
  std::cout << "[DEBUG] calculate_alternation_complexity called with "
            << iterators.size() << " iterators for variable " << (int)variable
            << std::endl;

  // TODO: Implement real logic
  // For now, just return a dummy value
  return 819;
}

/**
 * @brief Seek next value >= target in an iterator
 *
 * @param iterator The iterator to search in
 * @param variable The variable to search for
 * @param target The minimum value to find
 * @return The found value, or 0 if not found
 */
template <class ltj_iter_type, class var_type>
uint64_t seek_next_iterator(
    ltj_iter_type *iterator,
    var_type variable,
    uint64_t target
) {
  std::cout << "[DEBUG] seek_next_iterator called with target=" << target
            << " for variable " << (int)variable << std::endl;

  // TODO: Implement real logic using iterator->leap()
  // For now, just return a dummy value
  return target + 1;
}

} // namespace ltj

#endif // ALTERNATION_COMPLEXITY_HPP