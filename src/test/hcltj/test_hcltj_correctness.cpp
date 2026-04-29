// Correctness test: verifies that hcltj (hash path) produces the exact same
// result tuples as xcltj (leapfrog only) for every query in a test set.
// Also exercises the current H-CLTJ build pipeline and a serialize/load roundtrip.
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <query/ltj_algorithm.hpp>
#include <query/ltj_algorithm_hash.hpp>
#include <query/ltj_iterator_metatrie.hpp>
#include <query/ltj_iterator_metatrie_hash.hpp>
#include <results/results_collector.hpp>
#include <stdexcept>
#include <string>
#include <util/rdf_util.hpp>
#include <vector>
#include <veo/veo_adaptive.hpp>

using tuple_type = std::vector<std::pair<uint8_t, uint64_t>>;

namespace {

struct TempFileGuard {
    std::filesystem::path path;

    ~TempFileGuard() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

bool tuple_less(const tuple_type& a, const tuple_type& b) {
    for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) {
        if (a[i].first != b[i].first)
            return a[i].first < b[i].first;
        if (a[i].second != b[i].second)
            return a[i].second < b[i].second;
    }
    return a.size() < b.size();
}

std::vector<cltj::spo_triple> load_dataset(const std::string& dataset_path) {
    std::ifstream ifs(dataset_path);
    if (!ifs.good())
        throw std::runtime_error("dataset file not found: " + dataset_path);

    std::vector<cltj::spo_triple> triples;
    uint32_t s, p, o;
    cltj::spo_triple spo;
    while (ifs >> s >> p >> o) {
        spo[0] = s;
        spo[1] = p;
        spo[2] = o;
        triples.emplace_back(spo);
    }
    return triples;
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
    uint64_t n = std::min(res.size(), static_cast<uint64_t>(::util::results_collector<tuple_type>::buckets));
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

cltj::compact_ltj_metatrie_hash build_hcltj_index(
    const std::vector<cltj::spo_triple>& triples, uint32_t threshold
) {
    auto build_data = triples;
    cltj::compact_ltj_metatrie_hash index(build_data);

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

template <class x_algo, class h_algo>
int compare_query_results(
    const std::string& label,
    const std::vector<std::string>& queries,
    typename x_algo::index_scheme_type& xcltj,
    typename h_algo::index_scheme_type& hcltj
) {
    int failures = 0;

    for (size_t q = 0; q < queries.size(); ++q) {
        auto res_x = run_query<x_algo>(queries[q], xcltj);
        auto res_h = run_query<h_algo>(queries[q], hcltj);

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
            std::cout << "  PASS " << label << " q" << q << " (" << res_x.size()
                      << " results): " << queries[q] << std::endl;
        } else {
            std::cout << "  FAIL " << label << " q" << q << ": " << queries[q] << std::endl;
            std::cout << "    xcltj: " << res_x.size() << " results" << std::endl;
            for (auto& t : res_x)
                std::cout << "      " << tuple_to_string(t) << std::endl;
            std::cout << "    hcltj: " << res_h.size() << " results" << std::endl;
            for (auto& t : res_h)
                std::cout << "      " << tuple_to_string(t) << std::endl;
            failures++;
        }
    }

    return failures;
}

}  // namespace

int main() {
    using xcltj_index = cltj::compact_ltj_metatrie;
    using xcltj_iter = ltj::ltj_iterator_metatrie<xcltj_index, uint8_t, uint64_t>;
    using xcltj_algo =
        ltj::ltj_algorithm<xcltj_iter, ltj::veo::veo_adaptive<xcltj_iter, ltj::util::trait_size>>;

    using hcltj_index = cltj::compact_ltj_metatrie_hash;
    using hcltj_iter = ltj::ltj_iterator_metatrie_hash<hcltj_index, uint8_t, uint64_t>;
    using hcltj_algo =
        ltj::ltj_algorithm_hash<hcltj_iter, ltj::veo::veo_adaptive<hcltj_iter, ltj::util::trait_size>>;

    const std::string dataset_path = "data/data.txt";
    const uint32_t threshold = 2;

    // Queries that exercise the intersection path (2+ iterators per variable).
    // Triangle query (?v0 ?v1 ?v2 . ?v2 ?v1 ?v0) excluded: crashes on small
    // datasets in BOTH xcltj and hcltj due to pre-existing bug in
    // subtree_size_fixed1 (see journal 02).
    const std::vector<std::string> queries = {
        "?x 9 ?y . ?x 8 ?z",
        "?x 9 ?y . ?x 10 ?z",
        "?s ?p 3",
        "?x ?y 7 . ?x ?z 4",
        "?x1 9 ?x2 . ?x2 11 ?x3 . ?x3 9 ?x4 . ?x1 10 ?x4",
        "1 10 3",
        "?a 9 ?b . ?c 9 ?b",
    };

    try {
        auto dataset = load_dataset(dataset_path);
        auto x_data = dataset;
        xcltj_index xcltj(x_data);

        auto built_hcltj = build_hcltj_index(dataset, threshold);

        auto temp_index_path = std::filesystem::temp_directory_path() /
            ("cltj-test-hcltj-correctness-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".hcltj");
        TempFileGuard temp_file{temp_index_path};

        sdsl::store_to_file(built_hcltj, temp_index_path.string());

        hcltj_index loaded_hcltj;
        sdsl::load_from_file(loaded_hcltj, temp_index_path.string());

        int failures = 0;
        failures += compare_query_results<xcltj_algo, hcltj_algo>("built", queries, xcltj, built_hcltj);
        failures += compare_query_results<xcltj_algo, hcltj_algo>("loaded", queries, xcltj, loaded_hcltj);

        std::cout << std::endl;
        if (failures == 0)
            std::cout << "ALL PASS" << std::endl;
        else
            std::cout << failures << " FAILED" << std::endl;

        return failures > 0 ? 1 : 0;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return 1;
    }
}
