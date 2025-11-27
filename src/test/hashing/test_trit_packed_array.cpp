// Test suite for TritPackedArray
#include <hashing/storage/trit_packed_array.hpp>
#include <cassert>
#include <iostream>
#include <sstream>

using cltj::hashing::TritPackedArray;

/**
 * Basic test: initialization and default values
 */
void test_initialization() {
    std::cout << "Testing initialization... ";
    TritPackedArray arr;
    arr.initialize(5);

    assert(arr.size() == 5 && "Size should be 5");

    // All trits should be initialized to 0
    for (uint32_t i = 0; i < 5; i++) {
        assert(arr.get(i) == 0 && "Trit should be 0 after initialization");
    }
    std::cout << "PASSED\n";
}

/**
 * Test: basic write and read
 */
void test_basic_set_get() {
    std::cout << "Testing basic set/get... ";
    TritPackedArray arr;
    arr.initialize(5);

    // Write sequence [2, 1, 0, 2, 1]
    arr.set(0, 2);
    arr.set(1, 1);
    arr.set(2, 0);
    arr.set(3, 2);
    arr.set(4, 1);

    // Read and verify
    assert(arr.get(0) == 2 && "Trit at index 0 should be 2");
    assert(arr.get(1) == 1 && "Trit at index 1 should be 1");
    assert(arr.get(2) == 0 && "Trit at index 2 should be 0");
    assert(arr.get(3) == 2 && "Trit at index 3 should be 2");
    assert(arr.get(4) == 1 && "Trit at index 4 should be 1");
    std::cout << "PASSED\n";
}

/**
 * Test: modification of existing values
 */
void test_modification() {
    std::cout << "Testing modification... ";
    TritPackedArray arr;
    arr.initialize(3);

    arr.set(0, 1);
    arr.set(1, 2);
    arr.set(2, 0);

    assert(arr.get(0) == 1 && "Trit at index 0 should be 1");
    assert(arr.get(1) == 2 && "Trit at index 1 should be 2");
    assert(arr.get(2) == 0 && "Trit at index 2 should be 0");

    // Modify values
    arr.set(0, 0);
    arr.set(1, 1);
    arr.set(2, 2);

    assert(arr.get(0) == 0 && "Trit at index 0 should be 0 after modification");
    assert(arr.get(1) == 1 && "Trit at index 1 should be 1 after modification");
    assert(arr.get(2) == 2 && "Trit at index 2 should be 2 after modification");
    std::cout << "PASSED\n";
}

/**
 * Test: multiple words (more than 20 trits)
 */
void test_multiple_words() {
    std::cout << "Testing multiple words... ";
    TritPackedArray arr;
    arr.initialize(50);  // 3 words needed (20 + 20 + 10)

    // Write pattern: index % 3
    for (uint32_t i = 0; i < 50; i++) {
        arr.set(i, i % 3);
    }

    // Verify pattern
    for (uint32_t i = 0; i < 50; i++) {
        assert(arr.get(i) == (i % 3) && "Pattern verification failed");
    }
    std::cout << "PASSED\n";
}

/**
 * Test: values at word boundaries (positions 19-20-21)
 */
void test_word_boundaries() {
    std::cout << "Testing word boundaries... ";
    TritPackedArray arr;
    arr.initialize(40);

    // Write specific values at word boundaries
    arr.set(19, 2);  // Last trit of first word
    arr.set(20, 1);  // First trit of second word
    arr.set(21, 0);  // Second trit of second word

    assert(arr.get(19) == 2 && "Trit at boundary 19 should be 2");
    assert(arr.get(20) == 1 && "Trit at boundary 20 should be 1");
    assert(arr.get(21) == 0 && "Trit at boundary 21 should be 0");
    std::cout << "PASSED\n";
}

/**
 * Test: serialization and loading
 */
void test_serialization() {
    std::cout << "Testing serialization... ";
    TritPackedArray arr1;
    arr1.initialize(10);

    // Write data
    for (uint32_t i = 0; i < 10; i++) {
        arr1.set(i, (i * 2) % 3);
    }

    // Serialize
    std::stringstream ss;
    arr1.serialize(ss, nullptr, "test");

    // Load into new array
    TritPackedArray arr2;
    arr2.load(ss);

    // Verify they are equal
    assert(arr2.size() == arr1.size() && "Size mismatch after load");
    for (uint32_t i = 0; i < arr1.size(); i++) {
        assert(arr2.get(i) == arr1.get(i) && "Value mismatch after load");
    }
    std::cout << "PASSED\n";
}

/**
 * Test: size in bytes
 */
void test_size_in_bytes() {
    std::cout << "Testing size_in_bytes... ";
    TritPackedArray arr;
    arr.initialize(100);  // 5 words needed

    size_t expected_size = sizeof(uint32_t) +  // n_
        5 * sizeof(uint32_t);  // 5 words

    assert(arr.size_in_bytes() == expected_size && "Size calculation incorrect");
    std::cout << "PASSED\n";
}

/**
 * Test: edge case n=0
 */
void test_edge_case_zero() {
    std::cout << "Testing edge case n=0... ";
    TritPackedArray arr;
    arr.initialize(0);

    assert(arr.size() == 0 && "Size should be 0");
    // Should not crash on get/set with empty array
    std::cout << "PASSED\n";
}

/**
 * Test: edge case n=1
 */
void test_edge_case_one() {
    std::cout << "Testing edge case n=1... ";
    TritPackedArray arr;
    arr.initialize(1);

    assert(arr.size() == 1 && "Size should be 1");
    assert(arr.get(0) == 0 && "Initial value should be 0");

    arr.set(0, 2);
    assert(arr.get(0) == 2 && "Should be able to set and get");

    arr.set(0, 1);
    assert(arr.get(0) == 1 && "Should be able to modify");
    std::cout << "PASSED\n";
}

/**
 * Test: edge case n=20 (exactly one word)
 */
void test_edge_case_exact_word() {
    std::cout << "Testing edge case n=20 (exact word)... ";
    TritPackedArray arr;
    arr.initialize(20);  // Exactly 1 word

    // Fill all positions
    for (uint32_t i = 0; i < 20; i++) {
        arr.set(i, i % 3);
    }

    // Verify all positions
    for (uint32_t i = 0; i < 20; i++) {
        assert(arr.get(i) == (i % 3) && "Pattern verification failed at exact word boundary");
    }

    // Test boundaries: first and last
    arr.set(0, 2);
    arr.set(19, 1);
    assert(arr.get(0) == 2 && "First position should work");
    assert(arr.get(19) == 1 && "Last position should work");
    std::cout << "PASSED\n";
}

/**
 * Test: edge case n=21 (just crosses word boundary)
 */
void test_edge_case_cross_boundary() {
    std::cout << "Testing edge case n=21 (crosses boundary)... ";
    TritPackedArray arr;
    arr.initialize(21);  // 2 words (20 + 1)

    // Fill all positions
    for (uint32_t i = 0; i < 21; i++) {
        arr.set(i, (i * 2) % 3);
    }

    // Verify all positions
    for (uint32_t i = 0; i < 21; i++) {
        assert(arr.get(i) == ((i * 2) % 3) && "Pattern verification failed");
    }

    // Test the boundary position
    arr.set(20, 2);
    assert(arr.get(20) == 2 && "First trit of second word should work");
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== TritPackedArray Tests ===\n\n";

    try {
        test_initialization();
        test_basic_set_get();
        test_modification();
        test_multiple_words();
        test_word_boundaries();
        test_serialization();
        test_size_in_bytes();
        test_edge_case_zero();
        test_edge_case_one();
        test_edge_case_exact_word();
        test_edge_case_cross_boundary();

        std::cout << "\n✅ All tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << "\n";
        return 1;
    }
}
