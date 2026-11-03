// root_bitmap against a std::set over random id sets (with runs that cross the 64-bit words and the
// 4096-bit blocks of the succ support), after copy, move and a serialization round trip; the
// preconditions on the ids; and a round trip of cltj_index_metatrie_rb built from data/data.txt.
// Run from the repository root.
#include <index/cltj_index_metatrie_rb.hpp>
#include <index/root_bitmap.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool cond, const std::string& what) {
    if (!cond) {
        std::cout << "FAIL: " << what << std::endl;
        ++failures;
    }
}

std::set<uint64_t> make_ids(std::mt19937_64& rng, uint64_t universe, double density) {
    std::set<uint64_t> ids;
    std::bernoulli_distribution keep(density);
    for (uint64_t c = 1; c < universe; ++c)
        if (keep(rng))
            ids.insert(c);
    ids.insert(universe - 1);
    return ids;
}

void compare(const cltj::root_bitmap& rb, const std::array<std::set<uint64_t>, 3>& ids, const std::string& label) {
    for (int st = 0; st < 3; ++st) {
        const auto role = static_cast<state_type>(st);
        const auto& set = ids[st];
        const std::string where = label + " role " + "spo"[st];
        check(rb.count(role) == set.size(), where + ": count");
        check(rb.universe(role) == *set.rbegin() + 1, where + ": universe");
        uint64_t rank = 0;
        for (uint64_t c = 0; c <= *set.rbegin() + 70; ++c) {
            const bool in = set.count(c) > 0;
            auto it = set.lower_bound(c);
            const uint64_t next = it == set.end() ? 0 : *it;
            if (rb.contains(role, c) != in || rb.next(role, c) != next ||
                (c <= *set.rbegin() + 1 && rb.position(role, c) != rank)) {
                check(false, where + ": c = " + std::to_string(c));
                return;
            }
            rank += in;
        }
    }
}

bool throws(const std::vector<uint64_t>& ids) {
    cltj::root_bitmap rb;
    try {
        rb.set(state_type::s, ids.begin(), ids.end());
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void test_random() {
    std::mt19937_64 rng(11);
    const std::vector<std::pair<uint64_t, double>> shapes = {
        {2, 1.0}, {65, 1.0}, {300, 0.5}, {10000, 0.01}, {20000, 0.0005}, {70000, 0.3}, {200000, 0.00002}
    };
    for (const auto& shape : shapes) {
        std::array<std::set<uint64_t>, 3> ids;
        cltj::root_bitmap rb;
        for (int st = 0; st < 3; ++st) {
            ids[st] = make_ids(rng, shape.first + 31 * st, shape.second);
            std::vector<uint64_t> v(ids[st].begin(), ids[st].end());
            rb.set(static_cast<state_type>(st), v.begin(), v.end());
        }
        const std::string label = "universe " + std::to_string(shape.first);
        compare(rb, ids, label);

        cltj::root_bitmap copied(rb);
        compare(copied, ids, label + " copy");
        cltj::root_bitmap assigned;
        assigned = copied;
        compare(assigned, ids, label + " copy assignment");
        cltj::root_bitmap moved(std::move(copied));
        compare(moved, ids, label + " move");

        std::stringstream ss;
        const uint64_t written = rb.serialize(ss);
        check(written == sdsl::size_in_bytes(rb), label + ": serialized bytes");
        cltj::root_bitmap loaded;
        loaded.load(ss);
        compare(loaded, ids, label + " loaded");
    }
}

void test_preconditions() {
    check(throws({}), "empty root is rejected");
    check(throws({0, 3}), "id 0 is rejected");
    check(throws({2, 2}), "repeated id is rejected");
    check(throws({5, 3}), "decreasing ids are rejected");
    check(!throws({1, 2, 9}), "increasing ids from 1 are accepted");
}

void test_index_round_trip() {
    std::ifstream ifs("data/data.txt");
    std::vector<cltj::spo_triple> D;
    uint32_t s, p, o;
    while (ifs >> s >> p >> o)
        D.push_back({s, p, o});
    check(!D.empty(), "data/data.txt is readable");
    if (D.empty())
        return;

    cltj::compact_ltj_metatrie_rb index(D);
    std::stringstream first;
    index.serialize(first);
    cltj::compact_ltj_metatrie_rb loaded;
    loaded.load(first);
    std::stringstream second;
    loaded.serialize(second);
    check(first.str() == second.str(), "index round trip gives the same bytes");

    std::stringstream x_stream;
    cltj::compact_ltj_metatrie x(D);
    x.serialize(x_stream);
    cltj::compact_ltj_metatrie_rb wrong;
    bool rejected = false;
    try {
        wrong.load(x_stream);
    } catch (const std::runtime_error& e) {
        rejected = std::string(e.what()).find("not an index with a format version") != std::string::npos;
    }
    check(rejected, "an X-CLTJ index is rejected");
}

} // namespace

int main() {
    test_random();
    test_preconditions();
    test_index_round_trip();
    std::cout << (failures ? "FAILED" : "ALL PASS") << std::endl;
    return failures ? 1 : 0;
}
