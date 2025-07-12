#ifndef ALTERNATION_COMPLEXITY_HPP
#define ALTERNATION_COMPLEXITY_HPP

#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

namespace ltj {

/**
 * @brief Represents an interval in the alternation complexity calculation
 */
struct Interval {
  uint64_t left;
  uint64_t right;

  Interval(uint64_t l, uint64_t r) : left(l), right(r) {
  }

  bool is_singleton() const {
    return left == right;
  }

  void print() const {
    if (is_singleton()) {
      std::cout << "{" << left << "}";
    } else {
      std::cout << "[" << left << ", " << right << ")";
    }
  }
};

// Constants for boundary values
constexpr uint64_t POS_INF = std::numeric_limits<uint64_t>::max();
constexpr uint64_t NEG_INF = 0; // In RDF context, 0 is never a valid ID

/**
 * @brief Seek next value >= target in an iterator
 *
 * @param iterator The iterator to search in
 * @param variable The variable to search for
 * @param target The minimum value to find
 * @return The found value, or POS_INF if not found
 */
template <class ltj_iter_type, class var_type>
uint64_t
seek_next(ltj_iter_type *iterator, var_type variable, uint64_t target) {
  auto result = iterator->leap(variable, target);
  return result == 0 ? POS_INF : result;
}

/**
 * @brief Calculate the minimal certificate of a k-way intersection
 *
 * @param iterators Vector of iterators for the k-way intersection
 * @param variable The variable being processed
 * @return Vector of intervals representing the minimal certificate
 */
template <class ltj_iter_type, class var_type>
std::vector<Interval> calculate_minimal_certificate(
    const std::vector<ltj_iter_type *> &iterators,
    var_type variable
) {
  std::vector<Interval> intervals;

  if (iterators.empty()) {
    return intervals;
  }

  uint64_t left_bound = NEG_INF;
  uint64_t right_bound = NEG_INF;
  bool previous_value_in_intersection = false;

  std::cout << "[DEBUG] Starting minimal certificate calculation" << std::endl;

  while (right_bound < POS_INF) {
    // Advance all iterators
    uint64_t value_to_seek = left_bound;
    if (previous_value_in_intersection && left_bound < POS_INF) {
      value_to_seek = left_bound + 1;
    }

    std::cout << "[DEBUG] Iteration: left_bound=" << left_bound
              << ", right_bound=" << right_bound
              << ", value_to_seek=" << value_to_seek << std::endl;

    // Find the maximum value among all iterators (greedy choice)
    uint64_t current_value = NEG_INF;
    for (auto *iter : iterators) {
      uint64_t val = seek_next(iter, variable, value_to_seek);
      if (val > current_value) {
        current_value = val;
      }
    }

    std::cout << "[DEBUG] Current value (max): " << current_value << std::endl;

    // If all iterators returned POS_INF, we're done
    if (current_value == POS_INF) {
      std::cout << "[DEBUG] All iterators exhausted, breaking" << std::endl;
      break;
    }

    // Check if element is in the intersection
    uint64_t min_value = POS_INF;
    for (auto *iter : iterators) {
      uint64_t val = seek_next(iter, variable, current_value);
      if (val < min_value) {
        min_value = val;
      }
    }
    bool in_intersection =
        (min_value == current_value) && (current_value != POS_INF);

    std::cout << "[DEBUG] min_value=" << min_value
              << ", in_intersection=" << in_intersection << std::endl;

    if (in_intersection) {
      // Add gap before singleton and singleton itself (always, like Python)
      intervals.emplace_back(left_bound, current_value);
      intervals.emplace_back(current_value, current_value);
      std::cout << "[DEBUG] Added gap [" << left_bound << ", " << current_value
                << ") and singleton [" << current_value << ", " << current_value
                << "]" << std::endl;
      left_bound = current_value;
      previous_value_in_intersection = true;
    } else {
      // Add interval
      right_bound = current_value;
      intervals.emplace_back(left_bound, right_bound);
      std::cout << "[DEBUG] Added interval [" << left_bound << ", "
                << right_bound << ")" << std::endl;
      left_bound = right_bound;
      previous_value_in_intersection = false;
    }

    // Reset iterators for next iteration
    for (auto *iter : iterators) {
      iter->leap_done();
    }
  }

  // Add final interval if needed
  if (left_bound < POS_INF) {
    intervals.emplace_back(left_bound, POS_INF);
    std::cout << "[DEBUG] Added final interval [" << left_bound << ", +∞)"
              << std::endl;
  }

  return intervals;
}

/**
 * @brief Calculate alternation complexity
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

  auto intervals = calculate_minimal_certificate(iterators, variable);
  int result = static_cast<int>(intervals.size());

  std::cout << "[DEBUG] Alternation complexity calculated: " << result
            << std::endl;

  // Optional: Print intervals for debugging
  std::cout << "[DEBUG] Intervals: ";
  for (size_t i = 0; i < intervals.size(); ++i) {
    intervals[i].print();
    if (i < intervals.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << std::endl;

  return result;
}

} // namespace ltj

#endif // ALTERNATION_COMPLEXITY_HPP