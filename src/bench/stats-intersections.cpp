// Runs every query on X-CLTJ or H-CLTJ and writes its intersection stats aggregated per query.
// Characterization only: with CLTJ_COLLECT_QUERY_STATS the printed times are not measurements.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <map>
#include <numeric>
#include <query/ltj_algorithm.hpp>
#include <query/ltj_algorithm_hash.hpp>
#include <query/ltj_iterator_metatrie.hpp>
#include <query/ltj_iterator_metatrie_hash.hpp>
#include <results/results_counter.hpp>
#include <set>
#include <sstream>
#include <string>
#include <util/file_util.hpp>
#include <util/rdf_util.hpp>
#include <veo/veo_adaptive.hpp>

namespace {

const char* path_name(ltj::IntersectionPath path) {
    switch (path) {
        case ltj::IntersectionPath::LEAPFROG:
            return "LEAPFROG";
        case ltj::IntersectionPath::MIXED:
            return "MIXED";
        case ltj::IntersectionPath::PURE_HASH:
            return "PURE_HASH";
        case ltj::IntersectionPath::LONELY_SEEK_ALL:
            return "LONELY_SEEK_ALL";
    }
    return "UNKNOWN";
}

uint64_t bit_width(uint64_t x) { return x == 0 ? 0 : 64 - __builtin_clzll(x); }

// depth, var, path, k, hashed, large, min_bw, alt_bw
using group_key = std::array<uint64_t, 8>;

struct group_sums {
    uint64_t n = 0;
    uint64_t min = 0;
    uint64_t max = 0;
    uint64_t sizes = 0;
    uint64_t result = 0;
    uint64_t candidates = 0;
    uint64_t alternation = 0;
};

template <class index_type, class algorithm_type>
void run(
    const std::string& index_file,
    const std::string& queries_file,
    const uint64_t timeout,
    const uint64_t threshold,
    const std::string& out_file,
    const std::set<uint64_t>& skip
) {
    std::vector<std::string> queries;
    if (!::util::file::get_file_content(queries_file, queries)) {
        std::cerr << "cannot read " << queries_file << std::endl;
        std::exit(1);
    }

    index_type index;
    sdsl::load_from_file(index, index_file);
    std::cout << "Index loaded: " << sdsl::size_in_bytes(index) << " bytes." << std::endl;

    std::ofstream out(out_file);
    if (!out) {
        std::cerr << "cannot write " << out_file << std::endl;
        std::exit(1);
    }
    out << "query,depth,var,path,k,hashed,large,min_bw,alt_bw,"
           "n,sum_min,sum_max,sum_sizes,sum_result,sum_candidates,sum_alt\n";

    for (uint64_t q = 0; q < queries.size(); ++q) {
        if (skip.count(q))
            continue;
        auto query = ::util::rdf::ids::get_query(queries[q]);
        std::map<group_key, group_sums> groups;
        ::util::results_counter res;

        auto start = std::chrono::high_resolution_clock::now();
        algorithm_type ltj(&query, &index);
        ltj.set_stats_sink([&](const ltj::IntersectionStats& s) {
            const auto& sizes = s.list_sizes;
            uint64_t large = std::count_if(sizes.begin(), sizes.end(), [&](uint64_t n) { return n >= threshold; });
            group_key key = {
                static_cast<uint64_t>(s.depth),
                s.variable_id,
                static_cast<uint64_t>(s.path),
                s.k(),
                s.hashed_iterators,
                large,
                bit_width(s.min_list_size()),
                bit_width(static_cast<uint64_t>(s.alternation_complexity))
            };
            auto& g = groups[key];
            g.n++;
            g.min += s.min_list_size();
            g.max += s.max_list_size();
            g.sizes += std::accumulate(sizes.begin(), sizes.end(), uint64_t{0});
            g.result += s.result_size;
            g.candidates += s.candidates;
            g.alternation += s.alternation_complexity;
        });
        ltj.join(res, 0, timeout);
        auto stop = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        std::cout << q << ";" << res.size() << ";" << time << std::endl;

        for (const auto& [key, g] : groups) {
            out << q << ',' << key[0] << ',' << key[1] << ',' << path_name(static_cast<ltj::IntersectionPath>(key[2]));
            for (size_t i = 3; i < key.size(); ++i)
                out << ',' << key[i];
            out << ',' << g.n << ',' << g.min << ',' << g.max << ',' << g.sizes << ',' << g.result << ','
                << g.candidates << ',' << g.alternation << '\n';
        }
        out.flush();
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 7) {
        std::cout << "Usage: " << argv[0]
                  << " <xcltj|hcltj> <index> <queries> <timeout> <threshold> <out.csv> [skip_ids]" << std::endl;
        std::cout << "  threshold  only classifies lists into the `large` column (size >= threshold)" << std::endl;
        std::cout << "  skip_ids   comma-separated 0-based query ids to leave out" << std::endl;
        return 1;
    }

    const std::string system = argv[1];
    const std::string index = argv[2];
    const std::string queries = argv[3];
    const uint64_t timeout = std::stoull(argv[4]);
    const uint64_t threshold = std::stoull(argv[5]);
    const std::string out = argv[6];
    std::set<uint64_t> skip;
    if (argc > 7) {
        std::stringstream ss(argv[7]);
        std::string id;
        while (std::getline(ss, id, ','))
            skip.insert(std::stoull(id));
    }

    using x_index = cltj::compact_ltj_metatrie;
    using x_iter = ltj::ltj_iterator_metatrie<x_index, uint8_t, uint64_t>;
    using x_algo = ltj::ltj_algorithm<x_iter, ltj::veo::veo_adaptive<x_iter, ltj::util::trait_distinct>>;

    using h_index = cltj::compact_ltj_metatrie_hash;
    using h_iter = ltj::ltj_iterator_metatrie_hash<h_index, uint8_t, uint64_t>;
    using h_algo = ltj::ltj_algorithm_hash<h_iter, ltj::veo::veo_adaptive<h_iter, ltj::util::trait_distinct>>;

    if (system == "xcltj") {
        run<x_index, x_algo>(index, queries, timeout, threshold, out, skip);
    } else if (system == "hcltj") {
        run<h_index, h_algo>(index, queries, timeout, threshold, out, skip);
    } else {
        std::cerr << "unknown system: " << system << std::endl;
        return 1;
    }
    return 0;
}
