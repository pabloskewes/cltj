// util::radix_sort_high32 against std::stable_sort by the high 32 bits, over random inputs whose keys
// use 1 to 4 bytes, with repeated and distinct keys; and, for distinct keys, the packed (key, slot)
// order against std::sort of (key, beg + slot) pairs, which is the order of LTJ26_SORTED_SCAN.
#include <util/radix_sort.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

int failures = 0;

void check(bool cond, const std::string& what) {
    if (!cond) {
        std::cout << "FAIL: " << what << std::endl;
        ++failures;
    }
}

std::vector<uint64_t> make_input(std::mt19937_64& rng, size_t n, uint64_t key_range, bool distinct) {
    std::vector<uint64_t> a;
    a.reserve(n);
    std::uniform_int_distribution<uint64_t> key(0, key_range - 1);
    std::unordered_set<uint64_t> seen;
    for (size_t i = 0; i < n; ++i) {
        uint64_t k = key(rng);
        if (distinct) {
            while (!seen.insert(k).second)
                k = key(rng);
        }
        a.push_back(k << 32 | static_cast<uint32_t>(i));
    }
    return a;
}

void test_against_stable_sort(std::mt19937_64& rng) {
    const size_t sizes[] = {0, 1, 2, 3, 17, 256, 1000, 100000, 1000000};
    const uint64_t ranges[] = {1, 2, 200, 1ULL << 8, 1ULL << 16, 1ULL << 24, 100000000, 1ULL << 32};
    std::vector<uint64_t> tmp;
    for (size_t n : sizes) {
        for (uint64_t range : ranges) {
            for (bool distinct : {false, true}) {
                if (distinct && range < 2 * n)
                    continue;
                std::vector<uint64_t> a = make_input(rng, n, range, distinct);
                std::vector<uint64_t> expected = a;
                std::stable_sort(expected.begin(), expected.end(), [](uint64_t x, uint64_t y) {
                    return (x >> 32) < (y >> 32);
                });
                util::radix_sort_high32(a, tmp);
                const std::string label = "n=" + std::to_string(n) + " range=" + std::to_string(range)
                                          + (distinct ? " distinct" : " repeated");
                check(a == expected, "stable order, " + label);
            }
        }
    }
}

void test_keys_only_in_top_byte(std::mt19937_64& rng) {
    std::vector<uint64_t> a, tmp;
    std::uniform_int_distribution<uint64_t> top(0, 255);
    for (uint32_t i = 0; i < 5000; ++i)
        a.push_back(top(rng) << 56 | i);
    std::vector<uint64_t> expected = a;
    std::stable_sort(expected.begin(), expected.end(), [](uint64_t x, uint64_t y) { return (x >> 32) < (y >> 32); });
    util::radix_sort_high32(a, tmp);
    check(a == expected, "keys only in the top byte");
}

void test_reused_scratch(std::mt19937_64& rng) {
    std::vector<uint64_t> tmp;
    for (size_t n : {100000, 10, 50000, 3}) {
        std::vector<uint64_t> a = make_input(rng, n, 1ULL << 32, false);
        std::vector<uint64_t> expected = a;
        std::stable_sort(expected.begin(), expected.end(), [](uint64_t x, uint64_t y) { return (x >> 32) < (y >> 32); });
        util::radix_sort_high32(a, tmp);
        check(a == expected, "scratch reused, n=" + std::to_string(n));
    }
}

// Keys come in slot order from the MPHF cursor; slots and positions are unique.
void test_same_order_as_pairs(std::mt19937_64& rng) {
    const uint64_t begs[] = {0, 12345, (1ULL << 32) + 7, 5000000000ULL};
    std::vector<uint64_t> tmp;
    for (uint64_t beg : begs) {
        for (size_t n : {1, 2, 4000, 300000}) {
            std::vector<uint64_t> keys = make_input(rng, n, 1ULL << 32, true);
            std::vector<std::pair<uint64_t, uint64_t>> pairs;
            std::vector<uint64_t> packed;
            for (size_t slot = 0; slot < n; ++slot) {
                const uint32_t key = keys[slot] >> 32;
                pairs.emplace_back(key, beg + slot);
                packed.push_back(uint64_t(key) << 32 | slot);
            }
            std::sort(pairs.begin(), pairs.end());
            util::radix_sort_high32(packed, tmp);
            bool same = packed.size() == pairs.size();
            for (size_t i = 0; same && i < n; ++i)
                same = (packed[i] >> 32) == pairs[i].first && beg + uint32_t(packed[i]) == pairs[i].second;
            check(same, "same order as std::sort of pairs, beg=" + std::to_string(beg) + " n=" + std::to_string(n));
        }
    }
}

}  // namespace

int main() {
    std::mt19937_64 rng(20261122);
    test_against_stable_sort(rng);
    test_keys_only_in_top_byte(rng);
    test_reused_scratch(rng);
    test_same_order_as_pairs(rng);
    if (failures) {
        std::cout << failures << " failures" << std::endl;
        return 1;
    }
    std::cout << "all radix_sort_high32 tests passed" << std::endl;
    return 0;
}
