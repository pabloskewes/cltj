// level_widths_labels against single_width_labels and a linear scan, on random L_plain made of
// sorted blocks of two levels with different widths: access(i) at every position, and seek(val, i, f)
// inside every block from its start, from intermediate positions and from its end, for labels that
// are present, absent, smaller than the whole block and larger than it. Also a serialization round trip.
#include <metatrie/plain_labels.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using cltj::level_widths_labels;
using cltj::single_width_labels;

namespace {

int failures = 0;
uint64_t seeks = 0;

void expect(bool cond, const std::string& what) {
    if (!cond) {
        if (failures < 20)
            std::cout << "  FAIL: " << what << "\n";
        failures++;
    }
}

struct Block {
    uint64_t start, deg;
};

std::pair<uint32_t, uint64_t> linear_seek(const std::vector<uint32_t>& L, uint32_t val, uint64_t i, uint64_t f) {
    for (uint64_t k = i; k <= f; k++)
        if (L[k] >= val)
            return {L[k], k};
    return {0, f + 1};
}

// Appends n_blocks sorted blocks of distinct labels in [lo, hi] to L.
void add_level(std::mt19937_64& rng, std::vector<uint32_t>& L, std::vector<Block>& blocks, uint64_t n_blocks,
               uint32_t lo, uint32_t hi, uint64_t max_deg) {
    for (uint64_t b = 0; b < n_blocks; b++) {
        uint64_t deg = 1 + rng() % max_deg;
        deg = std::min<uint64_t>(deg, uint64_t(hi - lo) + 1);
        std::vector<uint32_t> labels;
        while (labels.size() < deg) {
            labels.push_back(lo + uint32_t(rng() % (uint64_t(hi - lo) + 1)));
            std::sort(labels.begin(), labels.end());
            labels.erase(std::unique(labels.begin(), labels.end()), labels.end());
        }
        blocks.push_back({L.size(), deg});
        L.insert(L.end(), labels.begin(), labels.end());
    }
}

void check_seek(const std::vector<uint32_t>& L, const single_width_labels& one, const level_widths_labels& two,
                uint32_t val, uint64_t i, uint64_t f, const std::string& what) {
    seeks++;
    const auto expected = linear_seek(L, val, i, f);
    const auto a = one.seek(val, i, f);
    const auto b = two.seek(val, i, f);
    if (a != expected || b != expected) {
        std::ostringstream m;
        m << what << ": seek(" << val << ", " << i << ", " << f << ") linear {" << expected.first << ", "
          << expected.second << "} single {" << a.first << ", " << a.second << "} level {" << b.first << ", "
          << b.second << "}";
        expect(false, m.str());
    }
}

void check_case(uint64_t seed, uint64_t blocks2, uint64_t blocks3, uint64_t max_deg) {
    std::mt19937_64 rng(seed);
    std::vector<uint32_t> L;
    std::vector<Block> blocks;
    add_level(rng, L, blocks, blocks2, 1 + rng() % 4, 5000 + rng() % 4000, max_deg);
    const uint64_t n2 = L.size();
    add_level(rng, L, blocks, blocks3, 1 + rng() % 4, (1u << 28) + uint32_t(rng() % (1u << 28)), max_deg);
    const std::string what = "seed=" + std::to_string(seed) + " n2=" + std::to_string(n2) + " n=" +
                             std::to_string(L.size());

    sdsl::int_vector<> v1(L.size(), 0, 32), v2(L.size(), 0, 32);
    for (uint64_t k = 0; k < L.size(); k++)
        v1[k] = v2[k] = L[k];
    const single_width_labels one(std::move(v1), n2);
    const level_widths_labels two(std::move(v2), n2);

    expect(one.size() == L.size() && two.size() == L.size() && two.n2() == n2, what + ": sizes");
    expect(two.plain2().size() == n2 && two.plain3().size() == L.size() - n2, what + ": level sizes");
    for (uint64_t k = 0; k < L.size(); k++)
        if (one[k] != L[k] || two.access(k) != L[k] || two[k] != L[k])
            expect(false, what + ": access(" + std::to_string(k) + ")");

    for (const auto& b : blocks) {
        const uint64_t f = b.start + b.deg - 1;
        std::vector<uint64_t> starts = {b.start, f, b.start + b.deg / 2};
        for (int r = 0; r < 3; r++)
            starts.push_back(b.start + rng() % b.deg);
        for (uint64_t i : starts) {
            std::vector<uint32_t> vals = {0, L[b.start] > 0 ? L[b.start] - 1 : 0, L[f] + 1, UINT32_MAX};
            for (uint64_t k = b.start; k <= f; k++) {
                vals.push_back(L[k]);
                vals.push_back(L[k] + 1);
            }
            for (int r = 0; r < 8; r++)
                vals.push_back(L[b.start] + uint32_t(rng() % (uint64_t(L[f] - L[b.start]) + 2)));
            for (uint32_t val : vals)
                check_seek(L, one, two, val, i, f, what);
        }
    }

    std::stringstream ss;
    two.serialize(ss);
    level_widths_labels loaded;
    loaded.load(ss);
    std::stringstream again;
    loaded.serialize(again);
    expect(loaded == two && again.str() == ss.str(), what + ": serialization round trip");
}

} // namespace

int main() {
    uint64_t cases = 0;
    for (uint64_t seed = 1; seed <= 300; seed++) {
        std::mt19937_64 rng(seed * 7919);
        uint64_t blocks2 = rng() % 12, blocks3 = rng() % 12;
        if (seed % 10 == 1)
            blocks2 = 0;  // n2 = 0
        if (seed % 10 == 2)
            blocks3 = 0;  // n2 = |L_plain|
        const uint64_t max_deg = (seed % 3 == 0) ? 3 : (seed % 3 == 1 ? 40 : 700);
        check_case(seed, blocks2, blocks3, max_deg);
        cases++;
    }
    check_case(1001, 0, 0, 5);  // empty L_plain
    cases++;
    std::cout << cases << " cases, " << seeks << " seeks\n";
    std::cout << (failures == 0 ? "ALL PASS" : "FAILURES: " + std::to_string(failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
