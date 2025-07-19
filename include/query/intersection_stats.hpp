#ifndef INTERSECTION_STATS_HPP
#define INTERSECTION_STATS_HPP

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

namespace ltj {

/**
 * @brief Stores statistics for a single k-way set intersection operation (a
 * leapfrog join).
 *
 * This struct is designed to be memory-efficient by storing only essential
 * primary data. Derived statistics (like min, max, or average sizes) are
 * computed on-the-fly via member functions.
 */
struct IntersectionStats {

  // --- Contextual Data ---
  /// @brief The ID of the variable being eliminated in this step.
  uint8_t variable_id = 0;

  /// @brief Position in the Variable Elimination Order (VEO)
  int depth = 0;

  // --- Primary Data (Stored) ---

  /// @brief A vector containing the size of each of the k lists being
  /// intersected.
  std::vector<uint64_t> list_sizes;

  /// @brief The calculated alternation complexity for this intersection.
  int alternation_complexity = 0;

  /// @brief The number of elements in the resulting intersection set.
  uint64_t result_size = 0;

  /// @brief The number of "seeks" or iterations the leapfrog join required to
  /// complete.
  uint64_t leapfrog_seeks = 0;

  // --- Derived Statistics (Computed on-the-fly) ---

  size_t k() const {
    return list_sizes.size();
  }

  uint64_t min_list_size() const {
    if (list_sizes.empty()) {
      return 0;
    }
    return *std::min_element(list_sizes.begin(), list_sizes.end());
  }

  uint64_t max_list_size() const {
    if (list_sizes.empty()) {
      return 0;
    }
    return *std::max_element(list_sizes.begin(), list_sizes.end());
  }

  double avg_list_size() const {
    if (list_sizes.empty()) {
      return 0.0;
    }
    const double sum =
        std::accumulate(list_sizes.begin(), list_sizes.end(), 0.0);
    return sum / k();
  }
};

} // namespace ltj

#endif // INTERSECTION_STATS_HPP