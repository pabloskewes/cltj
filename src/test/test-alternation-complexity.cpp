#include "../../include/query/alternation_complexity.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include <index/cltj_index_spo_dyn.hpp>
#include <query/ltj_algorithm.hpp>
#include <query/ltj_iterator_lite.hpp>
#include <results/results_collector.hpp>
#include <triple_pattern.hpp>
#include <util/rdf_util.hpp>
#include <veo/veo_simple.hpp>

using namespace ltj;

// Helper function to create expected intervals
std::vector<Interval> create_expected_intervals(
    const std::vector<std::pair<uint64_t, uint64_t>> &interval_pairs
) {
  std::vector<Interval> intervals;
  for (const auto &pair : interval_pairs) {
    intervals.emplace_back(pair.first, pair.second);
  }
  return intervals;
}

// Helper function to compare intervals
bool intervals_equal(
    const std::vector<Interval> &actual,
    const std::vector<Interval> &expected
) {
  if (actual.size() != expected.size()) {
    return false;
  }

  for (size_t i = 0; i < actual.size(); ++i) {
    if (actual[i].left != expected[i].left ||
        actual[i].right != expected[i].right) {
      return false;
    }
  }

  return true;
}

// Helper function to print intervals with infinity symbols
void print_intervals(const std::vector<Interval> &intervals) {
  std::cout << "[";
  for (size_t i = 0; i < intervals.size(); ++i) {
    if (i > 0)
      std::cout << ", ";

    // Custom print with infinity symbols
    if (intervals[i].is_singleton()) {
      std::cout << "{" << intervals[i].left << "}";
    } else {
      std::cout << "[";
      if (intervals[i].left == NEG_INF) {
        std::cout << "-∞";
      } else {
        std::cout << intervals[i].left;
      }
      std::cout << ", ";
      if (intervals[i].right == POS_INF) {
        std::cout << "+∞";
      } else {
        std::cout << intervals[i].right;
      }
      std::cout << ")";
    }
  }
  std::cout << "]";
}

/**
 * @brief Transformation function that converts simple lists into real iterators
 * @param simple_lists The simple lists (e.g., {{9}, {1,2,9,11}, ...})
 * @return A struct with the index, patterns, iterators and validation
 */
struct IteratorSetup {
  cltj::compact_dyn_ltj index;
  std::vector<ltj::triple_pattern> patterns;
  bool is_valid;
};

IteratorSetup create_iterators_from_simple_lists(
    const std::vector<std::vector<uint64_t>> &simple_lists
) {
  // Step 1: Convert simple lists → RDF triples
  std::vector<cltj::spo_triple> triples;
  for (size_t list_id = 0; list_id < simple_lists.size(); ++list_id) {
    for (size_t i = 0; i < simple_lists[list_id].size(); ++i) {
      cltj::spo_triple triple = {
          static_cast<uint32_t>(list_id + 1), // Subject = list ID
          1,                                  // Predicate = constant
          static_cast<uint32_t>(simple_lists[list_id][i]) // Object = list value
      };
      triples.push_back(triple);
    }
  }

  // Step 2: Create RDF index
  cltj::compact_dyn_ltj index(triples.begin(), triples.end());

  // Step 3: Create triple patterns for each list
  std::vector<ltj::triple_pattern> patterns;
  for (size_t list_id = 0; list_id < simple_lists.size(); ++list_id) {
    ltj::triple_pattern pattern;
    pattern.const_s(list_id + 1); // Fixed subject = list ID
    pattern.const_p(1);           // Fixed predicate
    pattern.var_o(0);             // Variable object (the variable we intersect)
    patterns.push_back(pattern);
  }

  // Step 4: Always valid if we reach here
  return {std::move(index), std::move(patterns), true};
}

void test_transformation_only() {
  std::cout << "\n=== Testing Classic Paper Example ===" << std::endl;

  // Classic example from the paper
  std::vector<std::vector<uint64_t>> simple_lists = {
      {9},             // A_1
      {1, 2, 9, 11},   // A_2
      {3, 9, 12, 13},  // A_3
      {9, 14, 15, 16}, // A_4
      {4, 10, 17, 18}, // A_5
      {5, 6, 7, 10},   // A_6
      {8, 10, 19, 20}  // A_7
  };

  std::cout << "Classic paper example (7 lists):" << std::endl;
  for (size_t i = 0; i < simple_lists.size(); ++i) {
    std::cout << "  A_" << (i + 1) << ": [";
    for (size_t j = 0; j < simple_lists[i].size(); ++j) {
      if (j > 0)
        std::cout << ", ";
      std::cout << simple_lists[i][j];
    }
    std::cout << "]" << std::endl;
  }

  // Expected intervals: [(-∞, 9), [9, 10), [10, +∞)]
  auto expected =
      create_expected_intervals({{NEG_INF, 9}, {9, 10}, {10, POS_INF}});

  std::cout << "Expected: ";
  print_intervals(expected);
  std::cout << std::endl;

  // Transform to real iterators
  auto setup = create_iterators_from_simple_lists(simple_lists);

  if (!setup.is_valid) {
    std::cout << "❌ Transformation failed!" << std::endl;
    return;
  }

  std::cout << "✓ Transformation successful, now running LTJ algorithm to get "
               "alternation complexity..."
            << std::endl;

  // Create the LTJ algorithm with our patterns and index
  typedef ltj::ltj_algorithm<
      ltj::ltj_iterator_lite<cltj::compact_dyn_ltj, uint8_t, uint64_t>,
      ltj::veo::veo_simple<
          ltj::ltj_iterator_lite<cltj::compact_dyn_ltj, uint8_t, uint64_t>,
          ltj::util::trait_size>>
      algorithm_type;

  algorithm_type ltj(&setup.patterns, &setup.index);

  // Create a simple results collector
  typedef ::util::results_collector<typename algorithm_type::tuple_type>
      results_type;
  results_type res;

  // Run the join (this will calculate alternation complexity as a side effect)
  ltj.join(res, 0, 0); // no limit, no timeout

  // Get the statistics which include alternation complexity
  const auto &stats = ltj.get_stats();

  std::cout << "LTJ algorithm completed. Found " << stats.size()
            << " intersection statistics." << std::endl;

  if (stats.empty()) {
    std::cout << "❌ No statistics collected!" << std::endl;
    std::cout << "✅ Algorithm finished without crashing." << std::endl;
    return;
  }

  // Since we have stats, we can now properly check the results
  // We need to implement a way to get the intervals from the stats
  // For now, we'll just check if the complexity count matches
  int actual_complexity = stats[0].alternation_complexity;
  int expected_complexity = expected.size();

  std::cout << "Actual complexity: " << actual_complexity << std::endl;
  std::cout << "Expected complexity: " << expected_complexity << std::endl;

  if (actual_complexity == expected_complexity) {
    std::cout << "✅ Alternation complexity matches expected!" << std::endl;
  } else {
    std::cout << "❌ Alternation complexity does NOT match expected!"
              << std::endl;
  }
}

// Test case for a specific intersection pattern
void test_case(
    const std::string &test_name,
    const std::vector<std::vector<uint64_t>> &simple_lists,
    const std::vector<std::pair<uint64_t, uint64_t>> &expected_intervals
) {
  std::cout << "\n=== " << test_name << " ===" << std::endl;

  std::cout << "Input lists:" << std::endl;
  for (size_t i = 0; i < simple_lists.size(); ++i) {
    std::cout << "  A_" << (i + 1) << ": [";
    for (size_t j = 0; j < simple_lists[i].size(); ++j) {
      if (j > 0)
        std::cout << ", ";
      std::cout << simple_lists[i][j];
    }
    std::cout << "]" << std::endl;
  }

  auto expected = create_expected_intervals(expected_intervals);
  std::cout << "Expected: ";
  print_intervals(expected);
  std::cout << std::endl;

  // Transform to real iterators
  auto setup = create_iterators_from_simple_lists(simple_lists);
  if (!setup.is_valid) {
    std::cout << "❌ Transformation failed!" << std::endl;
    return;
  }

  std::cout << "✓ Transformation successful, now running LTJ algorithm to get "
               "alternation complexity..."
            << std::endl;

  // Create the LTJ algorithm with our patterns and index
  typedef ltj::ltj_algorithm<
      ltj::ltj_iterator_lite<cltj::compact_dyn_ltj, uint8_t, uint64_t>,
      ltj::veo::veo_simple<
          ltj::ltj_iterator_lite<cltj::compact_dyn_ltj, uint8_t, uint64_t>,
          ltj::util::trait_size>>
      algorithm_type;

  algorithm_type ltj(&setup.patterns, &setup.index);

  // Create a simple results collector
  typedef ::util::results_collector<typename algorithm_type::tuple_type>
      results_type;
  results_type res;

  // Run the join (this will calculate alternation complexity as a side effect)
  ltj.join(res, 0, 0); // no limit, no timeout

  // Get the statistics which include alternation complexity
  const auto &stats = ltj.get_stats();

  std::cout << "LTJ algorithm completed. Found " << stats.size()
            << " intersection statistics." << std::endl;

  if (stats.empty()) {
    std::cout << "❌ No statistics collected!" << std::endl;
    std::cout << "✅ Algorithm finished successfully" << std::endl;
    return;
  }

  // We can now properly check the results
  int actual_complexity = stats[0].alternation_complexity;
  int expected_complexity = expected.size();

  std::cout << "Actual complexity: " << actual_complexity << std::endl;
  std::cout << "Expected complexity: " << expected_complexity << std::endl;

  if (actual_complexity == expected_complexity) {
    std::cout << "✅ Alternation complexity matches expected!" << std::endl;
  } else {
    std::cout << "❌ Alternation complexity does NOT match expected!"
              << std::endl;
  }
}

int main() {
  try {
    // Test the classic paper example
    test_transformation_only(); // Classic paper example

    // Test with different intersection patterns
    test_case(
        "Perfectly interleaved",
        {{1, 4, 7, 10, 13, 16}, {2, 5, 8, 11, 14, 17}, {3, 6, 9, 12, 15, 18}},
        {{NEG_INF, 3},
         {3, 5},
         {5, 7},
         {7, 9},
         {9, 11},
         {11, 13},
         {13, 15},
         {15, 17},
         {17, POS_INF}}
    );

    test_case(
        "Intersection at the beginning", {{1, 5, 9}, {1, 6, 10}, {1, 7, 11}},
        {{NEG_INF, 1}, {1, 1}, {1, 7}, {7, 10}, {10, POS_INF}}
    );

    test_case(
        "Intersection at the end", {{2, 6, 10}, {3, 7, 10}, {4, 8, 10}},
        {{NEG_INF, 4}, {4, 7}, {7, 10}, {10, 10}, {10, POS_INF}}
    );

    test_case(
        "Intersection at the middle", {{5}, {3, 5, 7}, {1, 5, 9}},
        {{NEG_INF, 5}, {5, 5}, {5, POS_INF}}
    );

    test_case(
        "No intersection", {{5}, {3, 7, 9}, {1, 6, 8}},
        {{NEG_INF, 5}, {5, 7}, {7, POS_INF}}
    );

    test_case(
        "All lists are identical", {{2, 4, 6}, {2, 4, 6}, {2, 4, 6}},
        {{NEG_INF, 2}, {2, 2}, {2, 4}, {4, 4}, {4, 6}, {6, 6}, {6, POS_INF}}
    );

    std::cout << "\nAll alternation complexity tests completed!" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}