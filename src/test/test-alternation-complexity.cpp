#include "../../include/query/alternation_complexity.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// Includes for real iterators
#include <index/cltj_index_spo_dyn.hpp>
#include <query/ltj_iterator_lite.hpp>
#include <triple_pattern.hpp>

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
  std::cout << "=== Transforming Simple Lists to Real Iterators ==="
            << std::endl;

  std::cout << "🎯 INPUT: " << simple_lists.size() << " listas a intersectar"
            << std::endl;

  for (size_t i = 0; i < simple_lists.size(); ++i) {
    std::cout << "  Lista " << i << ": {";
    for (size_t j = 0; j < simple_lists[i].size(); ++j) {
      if (j > 0)
        std::cout << ", ";
      std::cout << simple_lists[i][j];
    }
    std::cout << "}" << std::endl;
  }

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
  std::cout << "✓ Created RDF index with " << triples.size() << " triples"
            << std::endl;

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

  std::cout << "🔍 OUTPUT: " << iterators.size() << " iteradores reales creados"
            << std::endl;

  // Step 5: Validate that iterators contain exactly the expected lists
  std::cout << "\n=== Validating Iterator Contents ===" << std::endl;
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

    std::cout << "🔍 Iterador " << i << " contiene: {";
    for (size_t j = 0; j < iterator_values.size(); ++j) {
      if (j > 0)
        std::cout << ", ";
      std::cout << iterator_values[j];
    }
    std::cout << "}" << std::endl;

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
    std::cout << "\n🎉 All iterators validated successfully!" << std::endl;
    std::cout << "✅ TRANSFORMACIÓN LEGÍTIMA: inputs → iteradores validados"
              << std::endl;
  } else {
    std::cout << "\n❌ Iterator validation failed!" << std::endl;
  }

  return {
      std::move(index), std::move(patterns), std::move(iterators),
      std::move(iterator_ptrs), is_valid
  };
}

void test_classic_example() {
  std::cout << "=== Testing Classic Example with Transformation ==="
            << std::endl;

  // Define the simple lists for the classic example
  std::vector<std::vector<uint64_t>> simple_lists = {
      {9},             // A_1
      {1, 2, 9, 11},   // A_2
      {3, 9, 12, 13},  // A_3
      {9, 14, 15, 16}, // A_4
      {4, 10, 17, 18}, // A_5
      {5, 6, 7, 10},   // A_6
      {8, 10, 19, 20}  // A_7
  };

  // Transform to real iterators
  auto setup = create_iterators_from_simple_lists(simple_lists);

  if (!setup.is_valid) {
    std::cout << "❌ Setup validation failed - cannot continue" << std::endl;
    return;
  }

  // Now use the validated iterators to calculate alternation complexity
  std::cout << "\n=== Calculating Alternation Complexity ===" << std::endl;
  uint8_t variable = 0; // Object variable
  auto intervals =
      ltj::calculate_minimal_certificate(setup.iterator_ptrs, variable);

  std::cout << "Number of intervals: " << intervals.size() << std::endl;
  std::cout << "Alternation Complexity: " << intervals.size() << std::endl;

  // Print intervals
  for (size_t i = 0; i < intervals.size(); i++) {
    const auto &interval = intervals[i];
    std::cout << "Interval " << i << ": ";

    if (interval.left == 0) {
      std::cout << "(-∞, ";
    } else {
      std::cout << "[" << interval.left << ", ";
    }

    if (interval.right == std::numeric_limits<uint64_t>::max()) {
      std::cout << "+∞)";
    } else {
      std::cout << interval.right << ")";
    }

    if (interval.is_singleton()) {
      std::cout << " (singleton)";
    }
    std::cout << std::endl;
  }

  // Validate that we got some intervals (intersection should be empty)
  assert(intervals.size() > 0);

  std::cout << "✓ Classic example test completed successfully!" << std::endl;
}

void test_simple_case() {
  std::cout << "\n=== Testing Simple Case ===" << std::endl;

  // Simple case: A_1: [1, 3, 5], A_2: [2, 3, 4]
  // Intersection: {3}
  std::vector<std::vector<uint64_t>> simple_lists = {
      {1, 3, 5}, // A_1
      {2, 3, 4}  // A_2
  };

  auto setup = create_iterators_from_simple_lists(simple_lists);

  if (!setup.is_valid) {
    std::cout << "❌ Setup validation failed - cannot continue" << std::endl;
    return;
  }

  uint8_t variable = 0;
  auto intervals =
      ltj::calculate_minimal_certificate(setup.iterator_ptrs, variable);

  std::cout << "Alternation Complexity: " << intervals.size() << std::endl;

  for (size_t i = 0; i < intervals.size(); i++) {
    const auto &interval = intervals[i];
    std::cout << "Interval " << i << ": [" << interval.left << ", "
              << interval.right << ") ";
    if (interval.is_singleton())
      std::cout << "(singleton)";
    std::cout << std::endl;
  }

  std::cout << "✓ Simple case test completed!" << std::endl;
}

void test_transformation_only() {
  std::cout << "\n=== Testing ONLY the Transformation (No Algorithm) ==="
            << std::endl;

  // Define simple test lists
  std::vector<std::vector<uint64_t>> simple_lists = {
      {1, 3, 5},   // A_1
      {2, 3, 4},   // A_2
      {3, 6, 7, 8} // A_3
  };

  std::cout << "Input simple lists:" << std::endl;
  for (size_t i = 0; i < simple_lists.size(); ++i) {
    std::cout << "  A_" << (i + 1) << ": [";
    for (size_t j = 0; j < simple_lists[i].size(); ++j) {
      if (j > 0)
        std::cout << ", ";
      std::cout << simple_lists[i][j];
    }
    std::cout << "]" << std::endl;
  }

  // Transform to real iterators
  auto setup = create_iterators_from_simple_lists(simple_lists);

  if (setup.is_valid) {
    std::cout << "✅ TRANSFORMATION SUCCESSFUL!" << std::endl;
    std::cout << "   - Input lists correctly transformed to RDF triples"
              << std::endl;
    std::cout << "   - RDF index built successfully" << std::endl;
    std::cout << "   - Triple patterns created correctly" << std::endl;
    std::cout << "   - Real iterators instantiated and validated" << std::endl;
    std::cout << "   - Each iterator contains exactly the expected values"
              << std::endl;
  } else {
    std::cout << "❌ TRANSFORMATION FAILED!" << std::endl;
    std::cout << "   - Something went wrong in the transformation process"
              << std::endl;
  }

  // Test individual iterator functionality (simple leap test)
  std::cout << "\n=== Testing Individual Iterator Functionality ==="
            << std::endl;
  for (size_t i = 0; i < setup.iterators.size(); ++i) {
    std::cout << "Testing iterator " << i << ":" << std::endl;

    // Test leap with first expected value
    if (!simple_lists[i].empty()) {
      uint64_t first_val = simple_lists[i][0];
      uint64_t leap_result = setup.iterators[i].leap(0, first_val);
      std::cout << "  leap(0, " << first_val << ") = " << leap_result
                << std::endl;

      if (leap_result == first_val) {
        std::cout << "  ✅ leap() works correctly" << std::endl;
      } else {
        std::cout << "  ❌ leap() returned unexpected value" << std::endl;
      }

      // Reset after test
      setup.iterators[i].leap_done();
    }
  }

  std::cout << "\n🎯 Transformation validation completed!" << std::endl;
}

int main() {
  try {
    // First, test ONLY the transformation without algorithm
    test_transformation_only();

    // Uncomment these when transformation is validated:
    // test_classic_example();
    // test_simple_case();

    std::cout << "\n🎉 All tests completed!" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "❌ Test failed: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}