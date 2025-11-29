#pragma once
#include <cstdint>
#include <vector>
#include <cassert>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <sdsl/io.hpp>
#include <iostream>

namespace cltj {
namespace hashing {

/**
 * @brief Ceiling division: computes ceil(a / b)
 * @param a Numerator
 * @param b Denominator
 * @return Smallest integer >= a / b
 */
inline uint32_t ceil_div(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

/**
 * @brief Packed array of trits (base-3 digits: 0, 1, 2)
 * 
 * Stores trits efficiently by packing 20 trits into each uint32_t word.
 * Uses base-3 arithmetic: 3^20 = 3,486,784,401 < 2^32 = 4,294,967,296
 * 
 * Space: log₂(3) ≈ 1.585 bits per trit (vs 2 bits with int_vector<2>)
 */
class TritPackedArray {
  private:
    std::vector<uint32_t> words_;  // Each word stores 20 trits
    uint32_t n_;  // Total number of trits

    // Precomputed powers of 3: POW3[i] = 3^i for i in [0, 19]
    static constexpr uint32_t POW3[20] = {1,        3,        9,         27,        81,
                                          243,      729,      2187,      6561,      19683,
                                          59049,    177147,   531441,    1594323,   4782969,
                                          14348907, 43046721, 129140163, 387420489, 1162261467};

    static constexpr uint32_t TRITS_PER_WORD = 20;

    /**
     * @brief Extract a single trit from a packed word
     * 
     * To get trit at position i:
     *   1. Divide packed word by 3^i (stripping the trit at position i)
     *   2. Take result modulo 3 (get last "digit" after stripping)
     * 
     * Example: packed = 21 (represents [2,1,0])
     *   - unpack_trit(21, 0) = 21 / 3^0 % 3 = 21 % 3 = 0
     *   - unpack_trit(21, 1) = 21 / 3^1 % 3 = 7 % 3 = 1
     *   - unpack_trit(21, 2) = 21 / 3^2 % 3 = 2 % 3 = 2
     * 
     * @param packed The packed word containing 20 trits
     * @param trit_index Index within word [0, 19]
     * @return Trit value {0, 1, 2}
     */
    static uint8_t unpack_trit(uint32_t packed, uint32_t trit_index) {
        return (packed / POW3[trit_index]) % 3;
    }

    /**
     * @brief Update a single trit in a packed word
     * 
     * To change trit at position i to new_value:
     *   1. Extract old value: old = unpack_trit(packed, i)
     *   2. Remove old contribution: packed -= old * 3^i
     *   3. Add new contribution: packed += new_value * 3^i
     * 
     * Example: packed = 21 (represents [2,1,0]), change index 1 to 2:
     *   - old = 1
     *   - packed = 21 - 1*3 + 2*3 = 21 - 3 + 6 = 24
     *   - Result: 24 represents [2,2,0]
     * 
     * @param packed The packed word
     * @param trit_index Index within word [0, 19]
     * @param new_value New trit value {0, 1, 2}
     * @return Updated packed word
     */
    static uint32_t update_trit(uint32_t packed, uint32_t trit_index, uint8_t new_value) {
        assert(new_value <= 2 && "Trit value must be 0, 1, or 2");
        uint8_t old_value = unpack_trit(packed, trit_index);
        return packed - old_value * POW3[trit_index] + new_value * POW3[trit_index];
    }

  public:
    TritPackedArray() : n_(0) {}

    /**
     * @brief Initialize array to hold n trits
     * @param n Number of trits to store
     */
    void initialize(uint32_t n) {
        n_ = n;
        uint32_t num_words = ceil_div(n, TRITS_PER_WORD);
        words_.resize(num_words, 0);  // Initialize all words to 0 (all trits = 0)
    }

    /**
     * @brief Get trit value at index
     * @param index Global trit index [0, n-1]
     * @return Trit value {0, 1, 2}
     */
    uint8_t get(uint32_t index) const {
        assert(index < n_ && "Index out of bounds");
        uint32_t word_idx = index / TRITS_PER_WORD;
        uint32_t trit_idx = index % TRITS_PER_WORD;
        return unpack_trit(words_[word_idx], trit_idx);
    }

    /**
     * @brief Set trit value at index
     * @param index Global trit index [0, n-1]
     * @param value Trit value {0, 1, 2}
     */
    void set(uint32_t index, uint8_t value) {
        assert(index < n_ && "Index out of bounds");
        assert(value <= 2 && "Trit value must be 0, 1, or 2");
        uint32_t word_idx = index / TRITS_PER_WORD;
        uint32_t trit_idx = index % TRITS_PER_WORD;
        words_[word_idx] = update_trit(words_[word_idx], trit_idx, value);
    }

    /**
     * @brief Get number of trits stored
     */
    uint32_t size() const { return n_; }

    /**
     * @brief Get size in bytes
     */
    size_t size_in_bytes() const { return sizeof(n_) + words_.size() * sizeof(uint32_t); }

    /**
     * @brief Serialize to output stream
     */
    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        auto* child = sdsl::structure_tree::add_child(v, name, "TritPackedArray");
        size_t written = 0;
        written += sdsl::write_member(n_, out, child, "n");
        written += sdsl::serialize(words_, out, child, "words");
        sdsl::structure_tree::add_size(child, written);
        return written;
    }

    /**
     * @brief Load from input stream
     */
    void load(std::istream& in) {
        sdsl::read_member(n_, in);
        sdsl::load(words_, in);
    }
};

}  // namespace hashing
}  // namespace cltj
