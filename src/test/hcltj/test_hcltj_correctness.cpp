// Correctness test: verifies that hcltj (hash path) produces the exact same
// result tuples as xcltj (leapfrog only) for every query in a test set.
// Uses data/data.txt.xcltj and data/data.txt.hcltj (11 triples, threshold=2).
#include <algorithm>
#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <query/ltj_algorithm.hpp>
#include <query/ltj_algorithm_hash.hpp>
#include <query/ltj_iterator_metatrie.hpp>
#include <query/ltj_iterator_metatrie_hash.hpp>
#include <results/results_collector.hpp>
#include <string>
#include <util/rdf_util.hpp>
#include <vector>
#include <veo/veo_adaptive.hpp>

using tuple_type = std::vector<std::pair<uint8_t, uint64_t>>;

bool tuple_less(const tuple_type& a, const tuple_type& b) {
    for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) {
        if (a[i].first != b[i].first)
            return a[i].first < b[i].first;
        if (a[i].second != b[i].second)
            return a[i].second < b[i].second;
    }
    return a.size() < b.size();
}

template <class algorithm_type>
std::vector<tuple_type> run_query(
    const std::string& query_str, typename algorithm_type::index_scheme_type& index
) {
    auto query = ::util::rdf::ids::get_query(query_str);

    ::util::results_collector<tuple_type> res;
    algorithm_type ltj(&query, &index);
    ltj.join(res, 0, 600);

    std::vector<tuple_type> results;
    uint64_t n = std::min(res.size(), (uint64_t)::util::results_collector<tuple_type>::buckets);
    for (uint64_t i = 0; i < n; ++i)
        results.push_back(res[i]);

    for (auto& t : results)
        std::sort(t.begin(), t.end());
    std::sort(results.begin(), results.end(), tuple_less);
    return results;
}

std::string tuple_to_string(const tuple_type& t) {
    std::string s = "{";
    for (size_t i = 0; i < t.size(); ++i) {
        if (i > 0)
            s += ", ";
        s += "?" + std::to_string(t[i].first) + "=" + std::to_string(t[i].second);
    }
    return s + "}";
}

int main() {
    using xcltj_index = cltj::compact_ltj_metatrie;
    using xcltj_iter = ltj::ltj_iterator_metatrie<xcltj_index, uint8_t, uint64_t>;
    using xcltj_algo =
        ltj::ltj_algorithm<xcltj_iter, ltj::veo::veo_adaptive<xcltj_iter, ltj::util::trait_size>>;

    using hcltj_index = cltj::compact_ltj_metatrie_hash;
    using hcltj_iter = ltj::ltj_iterator_metatrie_hash<hcltj_index, uint8_t, uint64_t>;
    using hcltj_algo =
        ltj::ltj_algorithm_hash<hcltj_iter, ltj::veo::veo_adaptive<hcltj_iter, ltj::util::trait_size>>;

    xcltj_index xcltj;
    sdsl::load_from_file(xcltj, "data/data.txt.xcltj");

    hcltj_index hcltj;
    sdsl::load_from_file(hcltj, "data/data.txt.hcltj");

    // Queries that exercise the intersection path (2+ iterators per variable).
    // Triangle query (?v0 ?v1 ?v2 . ?v2 ?v1 ?v0) excluded: crashes on small
    // datasets in BOTH xcltj and hcltj due to pre-existing bug in
    // subtree_size_fixed1 (see journal 02).
    std::vector<std::string> queries = {
        "?x 9 ?y . ?x 8 ?z",
        "?x 9 ?y . ?x 10 ?z",
        "?s ?p 3",
        "?x ?y 7 . ?x ?z 4",
        "?x1 9 ?x2 . ?x2 11 ?x3 . ?x3 9 ?x4 . ?x1 10 ?x4",
        "1 10 3",
        "?a 9 ?b . ?c 9 ?b",
    };

    int failures = 0;
    for (size_t q = 0; q < queries.size(); ++q) {
        auto res_x = run_query<xcltj_algo>(queries[q], xcltj);
        auto res_h = run_query<hcltj_algo>(queries[q], hcltj);

        bool match = (res_x.size() == res_h.size());
        if (match) {
            for (size_t i = 0; i < res_x.size(); ++i) {
                if (res_x[i] != res_h[i]) {
                    match = false;
                    break;
                }
            }
        }

        if (match) {
            std::cout << "  PASS q" << q << " (" << res_x.size() << " results): " << queries[q] << std::endl;
        } else {
            std::cout << "  FAIL q" << q << ": " << queries[q] << std::endl;
            std::cout << "    xcltj: " << res_x.size() << " results" << std::endl;
            for (auto& t : res_x)
                std::cout << "      " << tuple_to_string(t) << std::endl;
            std::cout << "    hcltj: " << res_h.size() << " results" << std::endl;
            for (auto& t : res_h)
                std::cout << "      " << tuple_to_string(t) << std::endl;
            failures++;
        }
    }

    std::cout << std::endl;
    if (failures == 0)
        std::cout << "ALL PASS" << std::endl;
    else
        std::cout << failures << " FAILED" << std::endl;

    return failures > 0 ? 1 : 0;
}
