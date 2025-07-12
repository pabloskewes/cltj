#include "../../include/query/alternation_complexity.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// Includes for real iterators
#include <index/cltj_index_spo_dyn.hpp>
#include <query/ltj_iterator_lite.hpp>
#include <triple_pattern.hpp>

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

// Helper function to print intervals
void print_intervals(const std::vector<Interval> &intervals) {
  std::cout << "[";
  for (size_t i = 0; i < intervals.size(); ++i) {
    if (i > 0)
      std::cout << ", ";
    intervals[i].print();
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
  std::vector<ltj::ltj_iterator_lite<cltj::compact_dyn_ltj, uint8_t, uint64_t>>
      iterators;
  std::vector<
      ltj::ltj_iterator_lite<cltj::compact_dyn_ltj, uint8_t, uint64_t> *>
      iterator_ptrs;
  bool is_valid;
};

IteratorSetup create_iterators_from_simple_lists(
    const std::vector<std::vector<uint64_t>> &simple_lists
) {
  // Transform simple lists to real iterators

  std::cout << "Input: " << simple_lists.size() << " lists" << std::endl;

  // Step 1: Convert simple lists → RDF triples
  std::vector<cltj::spo_triple> triples;

  for (size_t list_id = 0; list_id < simple_lists.size(); ++list_id) {
    std::cout << "A_" << (list_id + 1) << ": [";
    for (size_t i = 0; i < simple_lists[list_id].size(); ++i) {
      if (i > 0)
        std::cout << ", ";
      std::cout << simple_lists[list_id][i];

      // Create triple: (list_id+1, 1, value)
      // This allows querying with pattern (list_id+1, 1, ?variable)
      cltj::spo_triple triple = {
          static_cast<uint32_t>(list_id + 1), // Subject = list ID
          1,                                  // Predicate = constant
          static_cast<uint32_t>(simple_lists[list_id][i]) // Object = list value
      };
      triples.push_back(triple);
    }
    std::cout << "]" << std::endl;
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

  // Step 4: Create real iterators
  typedef ltj::ltj_iterator_lite<cltj::compact_dyn_ltj, uint8_t, uint64_t>
      iterator_type;
  std::vector<iterator_type> iterators;
  std::vector<iterator_type *> iterator_ptrs;

  for (const auto &pattern : patterns) {
    iterators.emplace_back(&pattern, &index);
    iterator_ptrs.push_back(&iterators.back());
  }

  std::cout << "Created " << iterators.size() << " iterators" << std::endl;

  // Step 5: Validate that iterators contain exactly the expected lists
  bool is_valid = true;

  for (size_t i = 0; i < iterators.size(); ++i) {
    if (iterators[i].is_empty()) {
      std::cout << "❌ Iterator " << i << " is empty!" << std::endl;
      is_valid = false;
      continue;
    }

    // Get all values from iterator using seek_all
    std::vector<uint64_t> iterator_values;
    if (iterators[i].in_last_level()) {
      iterator_values = iterators[i].seek_all(0); // Variable 0 = object
    } else {
      // If not in last level, need to use leap to get values
      uint64_t val = iterators[i].leap(0); // Variable 0 = object
      while (val != 0) {
        iterator_values.push_back(val);
        val = iterators[i].leap(0, val + 1);
      }
      iterators[i].leap_done(); // Reset for next use
    }

    // Sort for comparison
    std::sort(iterator_values.begin(), iterator_values.end());

    // Compare with expected simple list
    std::vector<uint64_t> expected = simple_lists[i];
    std::sort(expected.begin(), expected.end());

    if (iterator_values == expected) {
      std::cout << "✓ Iterator " << i << " matches expected list!" << std::endl;
    } else {
      std::cout << "❌ Iterator " << i << " doesn't match expected list!"
                << std::endl;
      is_valid = false;
    }
  }

  if (is_valid) {
    std::cout << "All iterators validated successfully" << std::endl;
  } else {
    std::cout << "Iterator validation failed" << std::endl;
  }

  return {
      std::move(index), std::move(patterns), std::move(iterators),
      std::move(iterator_ptrs), is_valid
  };
}

void test_transformation_only() {
  std::cout << "\n=== Testing ONLY the Transformation (No Algorithm) ==="
            << std::endl;

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

  if (setup.is_valid) {
    std::cout << "Transformation successful" << std::endl;
  } else {
    std::cout << "Transformation failed" << std::endl;
  }

  std::cout << "Transformation validation completed" << std::endl;
}

// Generic test function to eliminate code duplication
void test_case_generic(
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

  auto setup = create_iterators_from_simple_lists(simple_lists);
  if (setup.is_valid) {
    std::cout << "Transformation successful" << std::endl;
  } else {
    std::cout << "Transformation failed" << std::endl;
  }
}

int main() {
  try {
    // Test various transformation cases
    test_transformation_only(); // Classic paper example

    // Test with different intersection patterns
    test_case_generic(
        "Common Intersection", {{2, 6, 10}, {3, 7, 10}, {4, 8, 10}},
        {{NEG_INF, 4}, {4, 7}, {7, 10}, {10, 10}, {10, POS_INF}}
    );

    test_case_generic(
        "Single Element Intersection", {{5}, {3, 5, 7}, {1, 5, 9}},
        {{NEG_INF, 5}, {5, 5}, {5, POS_INF}}
    );

    test_case_generic(
        "Empty Intersection", {{5}, {3, 7, 9}, {1, 6, 8}},
        {{NEG_INF, 5}, {5, 7}, {7, POS_INF}}
    );

    test_case_generic(
        "Identical Lists", {{2, 4, 6}, {2, 4, 6}, {2, 4, 6}},
        {{NEG_INF, 2}, {2, 2}, {2, 4}, {4, 4}, {4, 6}, {6, 6}, {6, POS_INF}}
    );

    std::cout << "\nAll transformation tests completed!" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}