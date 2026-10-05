// Checks the path, hashed iterator count, candidates and alternation that H-CLTJ records
// for intersections whose path is known from the degree of each node (hashed iff >= threshold).
#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <query/ltj_algorithm.hpp>
#include <query/ltj_algorithm_hash.hpp>
#include <query/ltj_iterator_metatrie.hpp>
#include <query/ltj_iterator_metatrie_hash.hpp>
#include <results/results_counter.hpp>
#include <set>
#include <string>
#include <util/rdf_util.hpp>
#include <vector>
#include <veo/veo_adaptive.hpp>

using x_index = cltj::compact_ltj_metatrie;
using x_iter = ltj::ltj_iterator_metatrie<x_index, uint8_t, uint64_t>;
using x_algo = ltj::ltj_algorithm<x_iter, ltj::veo::veo_adaptive<x_iter, ltj::util::trait_distinct>>;

using h_index = cltj::compact_ltj_metatrie_hash;
using h_iter = ltj::ltj_iterator_metatrie_hash<h_index, uint8_t, uint64_t>;
using h_algo = ltj::ltj_algorithm_hash<h_iter, ltj::veo::veo_adaptive<h_iter, ltj::util::trait_distinct>>;

using ltj::IntersectionPath;
using ltj::IntersectionStats;

namespace {

constexpr uint32_t threshold = 4;

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "  PASS " : "  FAIL ") << what << std::endl;
    if (!ok)
        failures++;
}

// Subjects per predicate: 1 -> {1..5} hashed, 2 -> {2..7} hashed, 3 -> {3,5}, 4 -> {5,6,9}.
// Subject 20 has 5 objects under predicate 5, so node (20, 5) is hashed.
// Predicate 6 is a pseudo-random graph over {30..69}, used for triangles.
std::vector<cltj::spo_triple> dataset() {
    std::vector<cltj::spo_triple> triples;
    auto add = [&](uint32_t s, uint32_t p, uint32_t o) { triples.push_back({s, p, o}); };
    for (uint32_t s = 1; s <= 5; ++s)
        add(s, 1, 100 + s);
    for (uint32_t s = 2; s <= 7; ++s)
        add(s, 2, 200 + s);
    for (uint32_t s : {3, 5})
        add(s, 3, 300 + s);
    for (uint32_t s : {5, 6, 9})
        add(s, 4, 400 + s);
    for (uint32_t o = 1; o <= 5; ++o)
        add(20, 5, o);
    uint32_t state = 12345;
    for (uint32_t a = 30; a < 70; ++a)
        for (uint32_t b = 30; b < 70; ++b) {
            state = state * 1103515245 + 12345;
            if (a != b && (state >> 16) % 5 == 0)
                add(a, 6, b);
        }
    return triples;
}

h_index build_hcltj(std::vector<cltj::spo_triple> triples) {
    h_index index(triples);
    constexpr int pairs[][2] = {
        {0, 1},
        {2, 3},
        {4, 5}
    };
    for (auto [full_i, part_i] : pairs) {
        auto* full = index.get_trie(full_i);
        auto* part = index.get_trie(part_i);
        full->build_hash_overlay(threshold, static_cast<uint32_t>(full_i));
        auto root_perm = full->extract_root_permutation();
        full->reorder_louds_by_mphf();
        part->build_hash_overlay(threshold, static_cast<uint32_t>(part_i));
        part->reorder_louds_by_mphf(root_perm);
    }
    return index;
}

template <class algo, class index_type>
uint64_t count_results(const std::string& query_str, index_type& index) {
    auto query = ::util::rdf::ids::get_query(query_str);
    ::util::results_counter res;
    algo ltj(&query, &index);
    ltj.join(res, 0, 600);
    return res.size();
}

template <class algo, class index_type>
std::vector<IntersectionStats> run(const std::string& query_str, index_type& index) {
    auto query = ::util::rdf::ids::get_query(query_str);
    ::util::results_counter res;
    algo ltj(&query, &index);
    ltj.join(res, 0, 600);
    return ltj.get_stats();
}

struct split_stats {
    std::vector<IntersectionStats> joined;
    std::vector<IntersectionStats> lonely;
};

split_stats split(const std::vector<IntersectionStats>& stats) {
    split_stats out;
    for (const auto& s : stats)
        (s.path == IntersectionPath::LONELY_SEEK_ALL ? out.lonely : out.joined).push_back(s);
    return out;
}

void check_hashed_matches_degree(const std::string& query, const std::vector<IntersectionStats>& stats) {
    bool ok = true;
    for (const auto& s : stats) {
        uint64_t large = 0;
        for (auto n : s.list_sizes)
            large += n >= threshold;
        ok = ok && s.hashed_iterators == large;
    }
    check(ok, query + ": hashed_iterators equals lists with size >= threshold");
}

void check_joined(
    const std::string& query,
    const split_stats& stats,
    IntersectionPath path,
    uint64_t k,
    uint64_t hashed,
    uint64_t result,
    uint64_t candidates
) {
    bool one = stats.joined.size() == 1;
    check(one, query + ": exactly one intersection recorded");
    if (!one)
        return;
    const auto& s = stats.joined[0];
    check(s.path == path, query + ": path");
    check(s.k() == k, query + ": k = " + std::to_string(k));
    check(s.hashed_iterators == hashed, query + ": hashed = " + std::to_string(hashed));
    check(s.result_size == result, query + ": result = " + std::to_string(result));
    check(s.candidates == candidates, query + ": candidates = " + std::to_string(candidates));
}

void check_lonely(const std::string& query, const split_stats& stats, uint64_t calls, uint64_t hashed, uint64_t size) {
    bool ok = stats.lonely.size() == calls;
    for (const auto& s : stats.lonely)
        ok = ok && s.k() == 1 && s.hashed_iterators == hashed && s.list_sizes[0] == size && s.result_size == size;
    check(
        ok,
        query + ": " + std::to_string(calls) + " lonely calls of size " + std::to_string(size) + ", hashed "
            + std::to_string(hashed)
    );
}

}  // namespace

int main() {
    auto triples = dataset();
    auto x_data = triples;
    x_index xcltj(x_data);
    h_index hcltj = build_hcltj(triples);

    {
        const std::string q = "?x 1 ?y . ?x 2 ?z";
        auto stats = run<h_algo>(q, hcltj);
        auto h = split(stats);
        check_joined(q, h, IntersectionPath::PURE_HASH, 2, 2, 4, 5);
        if (h.joined.size() == 1)
            check(h.joined[0].alternation_complexity == 0, q + ": no alternation on PURE_HASH");
        check_lonely(q, h, 8, 0, 1);
        check_hashed_matches_degree(q, stats);
    }
    {
        const std::string q = "?x 1 ?y . ?x 3 ?z";
        auto stats = run<h_algo>(q, hcltj);
        auto h = split(stats);
        check_joined(q, h, IntersectionPath::MIXED, 2, 1, 2, 2);
        // Over the sorted list {3, 5} alone: (-inf, 3) {3} [3, 5) {5} [5, +inf).
        if (h.joined.size() == 1)
            check(h.joined[0].alternation_complexity == 5, q + ": alternation over the sorted iterator only");
        check_lonely(q, h, 4, 0, 1);
        check_hashed_matches_degree(q, stats);
    }
    {
        const std::string q = "?x 1 ?y . ?x 2 ?z . ?x 4 ?w";
        auto stats = run<h_algo>(q, hcltj);
        auto h = split(stats);
        check_joined(q, h, IntersectionPath::MIXED, 3, 2, 1, 3);
        check_lonely(q, h, 3, 0, 1);
        check_hashed_matches_degree(q, stats);
    }
    {
        const std::string q = "?x 3 ?y . ?x 4 ?z";
        auto stats = run<h_algo>(q, hcltj);
        auto h = split(stats);
        check_joined(q, h, IntersectionPath::LEAPFROG, 2, 0, 1, 1);
        auto x = run<x_algo>(q, xcltj);
        if (h.joined.size() == 1 && x.size() == 1)
            check(
                h.joined[0].alternation_complexity == x[0].alternation_complexity,
                q + ": same alternation as X-CLTJ"
            );
        check_lonely(q, h, 2, 0, 1);
        check_hashed_matches_degree(q, stats);
    }
    {
        const std::string q = "?x 1 ?y";
        auto stats = run<h_algo>(q, hcltj);
        auto h = split(stats);
        check_joined(q, h, IntersectionPath::PURE_HASH, 1, 1, 5, 5);
        check_lonely(q, h, 5, 0, 1);
        check_hashed_matches_degree(q, stats);
    }
    {
        const std::string q = "20 5 ?o";
        auto stats = run<h_algo>(q, hcltj);
        auto h = split(stats);
        check(h.joined.empty(), q + ": no intersection recorded");
        check_lonely(q, h, 1, 1, 5);
        check_hashed_matches_degree(q, stats);
    }

    {
        // Intersections below depth 0 run once per parent binding, after the alternation of the
        // previous one: they must find the same results as without stats.
        std::set<std::pair<uint32_t, uint32_t>> edges;
        for (const auto& t : triples)
            if (t[1] == 6)
                edges.insert({t[0], t[2]});
        uint64_t triangles = 0;
        for (auto [a, b] : edges)
            for (auto [b2, c] : edges)
                triangles += b2 == b && edges.count({a, c});
        const std::string q = "?a 6 ?b . ?b 6 ?c . ?a 6 ?c";
        const std::string expected = std::to_string(triangles) + " triangles";
        check(count_results<x_algo>(q, xcltj) == triangles, q + ": X finds the " + expected);
        check(count_results<h_algo>(q, hcltj) == triangles, q + ": H finds the " + expected);
    }

    std::cout << std::endl << (failures == 0 ? "ALL PASS" : std::to_string(failures) + " FAILED") << std::endl;
    return failures > 0 ? 1 : 0;
}
