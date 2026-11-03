// root_bitmap against a std::set over random id sets (with runs that cross the 64-bit words and the
// 4096-bit blocks of the succ support), after copy, move and a serialization round trip; the
// preconditions on the ids; and round trips of cltj_index_metatrie_rb and cltj_index_metatrie_hash_rb
// built from data/data.txt. Run from the repository root.
#include <index/cltj_index_metatrie_hash_rb.hpp>
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

std::vector<cltj::spo_triple> read_data() {
    std::ifstream ifs("data/data.txt");
    std::vector<cltj::spo_triple> D;
    uint32_t s, p, o;
    while (ifs >> s >> p >> o)
        D.push_back({s, p, o});
    check(!D.empty(), "data/data.txt is readable");
    return D;
}

// Same steps as build-hcltj.
template <class Index> void build_hash_overlay(Index& index, uint32_t threshold, bool skip_root) {
    for (int full_i : {0, 2, 4}) {
        auto* full = index.get_trie(full_i);
        auto* part = index.get_trie(full_i + 1);
        full->build_hash_overlay(threshold, full_i, skip_root);
        auto root_perm = full->extract_root_permutation();
        full->reorder_louds_by_mphf();
        part->build_hash_overlay(threshold, full_i + 1);
        part->reorder_louds_by_mphf(root_perm);
    }
}

template <class Index> std::string load_error(const std::string& bytes) {
    std::stringstream ss(bytes);
    Index index;
    try {
        index.load(ss);
    } catch (const std::runtime_error& e) {
        return e.what();
    }
    return "";
}

void test_index_round_trip() {
    std::vector<cltj::spo_triple> D = read_data();
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

void test_hash_index_round_trip() {
    using c_index = cltj::cltj_index_metatrie<cltj::compact_metatrie_hash<cltj::overlay::StepTableOverlay>>;
    std::vector<cltj::spo_triple> D = read_data();
    if (D.empty())
        return;

    cltj::compact_ltj_metatrie_hash_rb index(D);
    build_hash_overlay(index, 2, true);
    std::stringstream first;
    index.serialize(first);
    cltj::compact_ltj_metatrie_hash_rb loaded;
    loaded.load(first);
    std::stringstream second;
    loaded.serialize(second);
    check(first.str() == second.str(), "hash index round trip gives the same bytes");
    for (int t : {0, 2, 4})
        check(loaded.get_trie(t)->dropped_root_labels() == loaded.get_trie(t)->root_degree() &&
                  !loaded.get_trie(t)->node_has_hash(0) && loaded.get_trie(t + 1)->dropped_root_labels() == 0,
              "trie " + std::to_string(t) + ": unhashed root out of L_plain, nothing dropped in its partial trie");

    const std::string version = "index format version ";
    check(load_error<c_index>(first.str()).find(version + "5, this binary reads version 3") != std::string::npos,
          "scheme C rejects the hash index with root bitmap");
    std::stringstream c_stream;
    c_index c(D);
    build_hash_overlay(c, 2, false);
    c.serialize(c_stream);
    check(load_error<cltj::compact_ltj_metatrie_hash_rb>(c_stream.str()).find(version + "3, this binary reads version 5") !=
              std::string::npos,
          "the hash index with root bitmap rejects scheme C");
    std::stringstream xrb_stream;
    cltj::compact_ltj_metatrie_rb(D).serialize(xrb_stream);
    check(load_error<cltj::compact_ltj_metatrie_hash_rb>(xrb_stream.str()).find(version + "4, this binary reads version 5") !=
              std::string::npos,
          "the hash index with root bitmap rejects X-CLTJ with root bitmap");

    cltj::compact_ltj_metatrie_hash_rb hashed_root(D);
    bool rejected = false;
    try {
        build_hash_overlay(hashed_root, 2, false);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected, "a hashed root is rejected when its block leaves L_plain");
}

} // namespace

int main() {
    test_random();
    test_preconditions();
    test_index_round_trip();
    test_hash_index_round_trip();
    std::cout << (failures ? "FAILED" : "ALL PASS") << std::endl;
    return failures ? 1 : 0;
}
