// Runs every query on X-CLTJ or H-CLTJ (with or without root bitmap) and writes its intersection
// stats aggregated per query. hcltj-rb and hcltj-c3 are always scheme C; xcltj-rb-wg is xcltj-rb with
// one width per level of L.
// Characterization only: with CLTJ_COLLECT_QUERY_STATS the printed times are not measurements.
//
// On the root-bitmap systems, --lists-output also writes one row per list and node, and --dump-node
// writes the operations on one node of one query: <prefix>.u32 with its labels, ascending;
// <prefix>.x.cltj-trace (leapfrog visits) and <prefix>.h.cltj-trace (the others) with, per visit in
// order, a uint32 m and m uint32 pairs (target, offset of the result in [0, n]) or (key, found);
// <prefix>.meta.txt with key=value lines.
#include <CLI11.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <index/cltj_index_metatrie.hpp>
#include <index/cltj_index_metatrie_hash_rb.hpp>
#include <index/cltj_index_metatrie_rb.hpp>
#include <iostream>
#include <map>
#include <numeric>
#include <query/ltj_algorithm.hpp>
#include <query/ltj_algorithm_hash.hpp>
#include <query/ltj_iterator_metatrie.hpp>
#include <query/ltj_iterator_metatrie_hash.hpp>
#include <query/ltj_iterator_metatrie_hash_rb.hpp>
#include <query/ltj_iterator_metatrie_rb.hpp>
#include <results/results_counter.hpp>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
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

const char* role_name(ltj::ListRole role) {
    switch (role) {
        case ltj::ListRole::LEAPFROG:
            return "leapfrog";
        case ltj::ListRole::SORTED:
            return "sorted";
        case ltj::ListRole::PROBE:
            return "probe";
        case ltj::ListRole::ENUM:
            return "enum";
    }
    return "unknown";
}

const char* trie_names[] = {"spo", "sop", "pos", "pso", "osp", "ops"};

struct list_options {
    std::string lists_out;
    uint64_t top_nodes = 0;
    bool dump = false;
    uint64_t dump_query = 0;
    ltj::ListNode dump_node;
    std::string dump_prefix;
    std::string system;
};

// depth, var, iter, path, role, k, hashed, trie, level, parent_pos, label1, label2, n (-1 on lists below
// the threshold, aggregated)
using list_key = std::array<int64_t, 13>;

struct list_key_hash {
    size_t operator()(const list_key& k) const {
        size_t h = 0;
        for (int64_t v : k)
            h = h * 1000003 ^ std::hash<int64_t>()(v);
        return h;
    }
};

using histogram = std::array<uint64_t, ltj::LOG2_BUCKETS>;

struct list_sums {
    uint64_t visits = 0;
    uint64_t sum_n = 0;
    uint64_t result = 0;
    uint64_t alternation = 0;
    uint64_t seeks = 0;
    uint64_t moved = 0;
    uint64_t sum_jump = 0;
    histogram jumps{};
    histogram first_jumps{};
    uint64_t sum_span = 0;
    histogram spans{};
    uint64_t probes = 0;
    uint64_t probes_found = 0;
    uint64_t enumerated = 0;
};

// b:count;b:count over the non-empty buckets
std::string sparse(const histogram& h) {
    std::string out;
    for (size_t b = 0; b < h.size(); ++b) {
        if (!h[b])
            continue;
        if (!out.empty())
            out += ';';
        out += std::to_string(b) + ':' + std::to_string(h[b]);
    }
    return out;
}

std::string node_name(const ltj::ListNode& node) {
    std::string name = std::string(trie_names[node.trie]) + ':' + std::to_string(node.level);
    if (node.label1 >= 0)
        name += ':' + std::to_string(node.label1);
    if (node.label2 >= 0)
        name += ':' + std::to_string(node.label2);
    return name;
}

ltj::ListNode parse_node(const std::string& text) {
    std::vector<std::string> parts;
    std::stringstream in(text);
    std::string part;
    while (std::getline(in, part, ':'))
        parts.push_back(part);
    ltj::ListNode node;
    const auto* trie = std::find(std::begin(trie_names), std::end(trie_names), parts.empty() ? "" : parts[0]);
    if (parts.size() < 2 || trie == std::end(trie_names))
        throw std::invalid_argument("--dump-node: expected trie:level[:label1[:label2]], got " + text);
    node.trie = trie - std::begin(trie_names);
    node.level = std::stoul(parts[1]);
    if (node.level < 1 || node.level > 3 || parts.size() != node.level + 1)
        throw std::invalid_argument("--dump-node: a node of level l takes l - 1 labels, got " + text);
    if (parts.size() > 2)
        node.label1 = std::stoll(parts[2]);
    if (parts.size() > 3)
        node.label2 = std::stoll(parts[3]);
    return node;
}

template <class index_type>
constexpr bool is_hash_index =
    std::is_same_v<index_type, cltj::compact_ltj_metatrie_hash_rb> || std::is_same_v<index_type, cltj::compact_ltj_metatrie_hash_c3>;

// Labels of the node's children, ascending.
template <class index_type>
std::vector<uint32_t> node_labels(const index_type& index, const ltj::ListNode& node, uint64_t n) {
    std::vector<uint32_t> labels;
    labels.reserve(n);
    if (node.level == 1) {
        const auto st = static_cast<state_type>(node.trie / 2);
        for (uint64_t v = index.root().next(st, 0); v != 0; v = index.root().next(st, v + 1))
            labels.push_back(v);
        return labels;
    }
    const auto* trie = index.get_trie(node.trie);
    if constexpr (is_hash_index<index_type>) {
        if (trie->node_has_hash(node.parent_pos)) {
            auto keys = trie->hash_keys(node.parent_pos);
            while (keys.next())
                labels.push_back(keys.key());
            std::sort(labels.begin(), labels.end());
            return labels;
        }
        const uint64_t off = trie->deleted_before(node.parent_pos);
        for (uint64_t i = 0; i < n; ++i)
            labels.push_back(trie->plain_label(node.begin + i, off));
    } else {
        for (uint64_t i = 0; i < n; ++i)
            labels.push_back(trie->label(node.begin + i));
    }
    return labels;
}

// Bits per label of the node's level in L; -1 for the root bitmap and for H.
template <class index_type>
int64_t label_width(const index_type& index, const ltj::ListNode& node) {
    if (node.level == 1)
        return -1;
    if constexpr (std::is_same_v<index_type, cltj::compact_ltj_metatrie_rb_wg>) {
        const auto& labels = index.get_trie(node.trie)->labels();
        return node.level == 2 ? labels.plain2().width() : labels.plain3().width();
    } else if constexpr (std::is_same_v<index_type, cltj::compact_ltj_metatrie_rb>) {
        return index.get_trie(node.trie)->labels().width();
    }
    return -1;
}

struct dump_state {
    std::ofstream x, h;
    uint64_t visits = 0, ops = 0;
    bool found = false;
    ltj::ListNode node;
    uint64_t n = 0;
    // "depth/var/iter" -> visits
    std::map<std::string, uint64_t> contexts;

    static void put(std::ofstream& out, uint32_t v) { out.write(reinterpret_cast<const char*>(&v), sizeof(v)); }

    void add(const ltj::IntersectionStats& s, size_t i, const std::string& prefix) {
        const auto& l = s.lists[i];
        auto& out = l.role == ltj::ListRole::LEAPFROG ? x : h;
        if (!out.is_open())
            out.open(prefix + (l.role == ltj::ListRole::LEAPFROG ? ".x.cltj-trace" : ".h.cltj-trace"), std::ios::binary);
        put(out, l.ops.size());
        for (const auto& [a, b] : l.ops) {
            put(out, a);
            put(out, b);
        }
        if (!found) {
            node = l.node;
            n = l.n;
            found = true;
        }
        ++visits;
        ops += l.ops.size();
        contexts[std::to_string(s.depth) + '/' + std::to_string(s.variable_id) + '/' + std::to_string(i)]++;
    }
};

template <class index_type, class algorithm_type>
void run(
    const std::string& index_file,
    const std::string& queries_file,
    const uint64_t timeout,
    const uint64_t threshold,
    const std::string& out_file,
    const std::set<uint64_t>& skip,
    const list_options& opt
) {
    constexpr bool names_nodes = ltj::names_list_nodes<typename algorithm_type::ltj_iter_type>::value;
    if (!names_nodes && (!opt.lists_out.empty() || opt.dump || opt.top_nodes)) {
        std::cerr << "--lists-output, --top-nodes and --dump-* need a root-bitmap system" << std::endl;
        std::exit(1);
    }

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

    std::ofstream lists_out;
    if (!opt.lists_out.empty()) {
        lists_out.open(opt.lists_out);
        if (!lists_out) {
            std::cerr << "cannot write " << opt.lists_out << std::endl;
            std::exit(1);
        }
        lists_out << "query,depth,var,iter,trie,level,parent_pos,label1,label2,n,path,role,k,hashed,"
                     "visits,sum_n,sum_result,sum_alt,seeks,moved,sum_jump,jump_hist,first_jump_hist,"
                     "sum_span,span_hist,probes,probes_found,enumerated\n";
    }
    const bool track_lists = !opt.lists_out.empty() || opt.top_nodes;

    for (uint64_t q = 0; q < queries.size(); ++q) {
        if (skip.count(q) || (opt.dump && q != opt.dump_query))
            continue;
        auto query = ::util::rdf::ids::get_query(queries[q]);
        std::map<group_key, group_sums> groups;
        std::unordered_map<list_key, list_sums, list_key_hash> lists;
        dump_state dump;
        ::util::results_counter res;

        auto start = std::chrono::high_resolution_clock::now();
        algorithm_type ltj(&query, &index);
        if constexpr (names_nodes)
            if (opt.dump)
                ltj.set_trace_node(opt.dump_node);
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

            for (size_t i = 0; i < s.lists.size(); ++i) {
                const auto& l = s.lists[i];
                if (l.trace)
                    dump.add(s, i, opt.dump_prefix);
                if (!track_lists)
                    continue;
                list_key lk = {
                    s.depth, s.variable_id, static_cast<int64_t>(i), static_cast<int64_t>(s.path),
                    static_cast<int64_t>(l.role), static_cast<int64_t>(s.k()), s.hashed_iterators,
                    -1, -1, -1, -1, -1, -1
                };
                if (l.n >= threshold) {
                    lk[7] = l.node.trie;
                    lk[8] = l.node.level;
                    lk[9] = l.node.parent_pos;
                    lk[10] = l.node.label1;
                    lk[11] = l.node.label2;
                    lk[12] = l.n;
                }
                auto& ls = lists[lk];
                ls.visits++;
                ls.sum_n += l.n;
                ls.result += s.result_size;
                ls.alternation += s.alternation_complexity;
                ls.seeks += l.seeks;
                ls.moved += l.moved;
                ls.sum_jump += l.sum_jump;
                for (size_t b = 0; b < ltj::LOG2_BUCKETS; ++b)
                    ls.jumps[b] += l.jumps[b];
                if (l.first_jump_bucket >= 0)
                    ls.first_jumps[l.first_jump_bucket]++;
                if (l.role == ltj::ListRole::LEAPFROG) {
                    ls.sum_span += l.cursor;
                    ls.spans[ltj::log2_bucket(l.cursor)]++;
                }
                ls.probes += l.probes;
                ls.probes_found += l.probes_found;
                ls.enumerated += l.enumerated;
            }
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

        if constexpr (names_nodes) {
            std::vector<std::pair<list_key, const list_sums*>> rows;
            for (const auto& [k, ls] : lists)
                rows.emplace_back(k, &ls);
            std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
            if (lists_out.is_open()) {
                for (const auto& [k, ls] : rows) {
                    lists_out << q << ',' << k[0] << ',' << k[1] << ',' << k[2] << ','
                              << (k[7] < 0 ? "-1" : trie_names[k[7]]);
                    for (size_t i = 8; i < 13; ++i)
                        lists_out << ',' << k[i];
                    lists_out << ',' << path_name(static_cast<ltj::IntersectionPath>(k[3])) << ','
                              << role_name(static_cast<ltj::ListRole>(k[4])) << ',' << k[5] << ',' << k[6] << ','
                              << ls->visits << ',' << ls->sum_n << ',' << ls->result << ',' << ls->alternation << ','
                              << ls->seeks << ',' << ls->moved << ',' << ls->sum_jump << ',' << sparse(ls->jumps)
                              << ',' << sparse(ls->first_jumps) << ',' << ls->sum_span << ',' << sparse(ls->spans)
                              << ',' << ls->probes << ',' << ls->probes_found << ',' << ls->enumerated << '\n';
                }
                lists_out.flush();
            }

            if (opt.top_nodes) {
                // trie, level, parent_pos, label1, label2, n -> visits, seeks, probes, enumerated
                std::map<std::array<int64_t, 6>, std::array<uint64_t, 4>> nodes;
                for (const auto& [k, ls] : rows) {
                    if (k[12] < 0)
                        continue;
                    auto& t = nodes[{k[7], k[8], k[9], k[10], k[11], k[12]}];
                    t[0] += ls->visits;
                    t[1] += ls->seeks;
                    t[2] += ls->probes;
                    t[3] += ls->enumerated;
                }
                std::vector<std::pair<std::array<int64_t, 6>, std::array<uint64_t, 4>>> top(nodes.begin(), nodes.end());
                std::stable_sort(top.begin(), top.end(), [](const auto& a, const auto& b) { return a.second[0] > b.second[0]; });
                top.resize(std::min<size_t>(top.size(), opt.top_nodes));
                for (const auto& [k, t] : top) {
                    ltj::ListNode node;
                    node.trie = k[0];
                    node.level = k[1];
                    node.label1 = k[3];
                    node.label2 = k[4];
                    std::cerr << "top;" << q << ';' << node_name(node) << ';' << k[2] << ';' << k[5] << ';' << t[0]
                              << ';' << t[1] << ';' << t[2] << ';' << t[3] << std::endl;
                }
            }

            if (opt.dump) {
                if (!dump.found) {
                    std::cerr << "--dump-node " << node_name(opt.dump_node) << " is not visited by query " << q
                              << std::endl;
                    std::exit(1);
                }
                const auto labels = node_labels(index, dump.node, dump.n);
                std::ofstream u32(opt.dump_prefix + ".u32", std::ios::binary);
                u32.write(reinterpret_cast<const char*>(labels.data()), labels.size() * sizeof(uint32_t));
                std::ofstream meta(opt.dump_prefix + ".meta.txt");
                const auto& first = *dump.contexts.begin();
                std::string depth = first.first.substr(0, first.first.find('/'));
                std::string rest = first.first.substr(first.first.find('/') + 1);
                std::string var = rest.substr(0, rest.find('/'));
                meta << "system=" << opt.system << "\nquery=" << q << "\ndepth=" << depth << "\nvar=" << var
                     << "\ntrie=" << trie_names[dump.node.trie] << "\nlevel=" << int(dump.node.level)
                     << "\nparent_pos=" << dump.node.parent_pos << "\nlabel1=" << dump.node.label1
                     << "\nlabel2=" << dump.node.label2 << "\nn=" << dump.n
                     << "\nwidth=" << label_width(index, dump.node) << "\nvisits=" << dump.visits
                     << "\nops=" << dump.ops << "\ncontexts=";
                for (auto it = dump.contexts.begin(); it != dump.contexts.end(); ++it)
                    meta << (it == dump.contexts.begin() ? "" : ";") << it->first << ':' << it->second;
                meta << '\n';
                if (labels.size() != dump.n) {
                    std::cerr << "node has " << labels.size() << " labels, expected " << dump.n << std::endl;
                    std::exit(1);
                }
            }
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    CLI::App app{"Per-query intersection stats of X-CLTJ or H-CLTJ (needs CLTJ_COLLECT_QUERY_STATS)"};
    std::string system, index, queries, out;
    uint64_t timeout = 600;
    uint64_t threshold = 4000;
    std::vector<uint64_t> skip_ids;
    list_options opt;
    std::string dump_node;
    app.add_option("system", system, "Index type")
        ->required()
        ->check(CLI::IsMember({"xcltj", "xcltj-rb", "xcltj-rb-wg", "hcltj", "hcltj-rb", "hcltj-c3"}));
    app.add_option("index", index, "Index file")->required();
    app.add_option("queries", queries, "Query file, one query per line")->required();
    app.add_option("-o,--output", out, "Output CSV")->required();
    app.add_option("--timeout", timeout, "Timeout per query in seconds")->capture_default_str();
    app.add_option("-t,--threshold", threshold, "Lists with at least this size count as `large`")
        ->capture_default_str();
    app.add_option("--skip", skip_ids, "Comma-separated 0-based query ids to leave out")->delimiter(',');
    app.add_option("--lists-output", opt.lists_out, "Per-list CSV: one row per node with n >= threshold");
    app.add_option("--top-nodes", opt.top_nodes, "Print to stderr the K most visited large nodes of each query");
    auto* dq = app.add_option("--dump-query", opt.dump_query, "Run only this query and dump --dump-node");
    auto* dn = app.add_option("--dump-node", dump_node, "Node to dump: trie:level[:label1[:label2]], e.g. pso:2:31");
    auto* dp = app.add_option("--dump-prefix", opt.dump_prefix, "Prefix of the dump files");
    dq->needs(dn, dp);
    dn->needs(dq, dp);
    dp->needs(dq, dn);
    CLI11_PARSE(app, argc, argv);
    opt.system = system;
    if (!dump_node.empty()) {
        opt.dump = true;
        try {
            opt.dump_node = parse_node(dump_node);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }
    }
    const std::set<uint64_t> skip(skip_ids.begin(), skip_ids.end());
    if (opt.dump && skip.count(opt.dump_query)) {
        std::cerr << "--dump-query " << opt.dump_query << " is in --skip" << std::endl;
        return 1;
    }

    using x_index = cltj::compact_ltj_metatrie;
    using x_iter = ltj::ltj_iterator_metatrie<x_index, uint8_t, uint64_t>;
    using x_algo = ltj::ltj_algorithm<x_iter, ltj::veo::veo_adaptive<x_iter, ltj::util::trait_distinct>>;

    using xrb_index = cltj::compact_ltj_metatrie_rb;
    using xrb_iter = ltj::ltj_iterator_metatrie_rb<xrb_index, uint8_t, uint64_t>;
    using xrb_algo = ltj::ltj_algorithm<xrb_iter, ltj::veo::veo_adaptive<xrb_iter, ltj::util::trait_distinct>>;

    using xwg_index = cltj::compact_ltj_metatrie_rb_wg;
    using xwg_iter = ltj::ltj_iterator_metatrie_rb<xwg_index, uint8_t, uint64_t>;
    using xwg_algo = ltj::ltj_algorithm<xwg_iter, ltj::veo::veo_adaptive<xwg_iter, ltj::util::trait_distinct>>;

    using h_index = cltj::compact_ltj_metatrie_hash;
    using h_iter = ltj::ltj_iterator_metatrie_hash<h_index, uint8_t, uint64_t>;
    using h_algo = ltj::ltj_algorithm_hash<h_iter, ltj::veo::veo_adaptive<h_iter, ltj::util::trait_distinct>>;

    using hrb_index = cltj::compact_ltj_metatrie_hash_rb;
    using hrb_iter = ltj::ltj_iterator_metatrie_hash_rb<hrb_index, uint8_t, uint64_t>;
    using hrb_algo = ltj::ltj_algorithm_hash<hrb_iter, ltj::veo::veo_adaptive<hrb_iter, ltj::util::trait_distinct>>;

    using hc3_index = cltj::compact_ltj_metatrie_hash_c3;
    using hc3_iter = ltj::ltj_iterator_metatrie_hash_rb<hc3_index, uint8_t, uint64_t>;
    using hc3_algo = ltj::ltj_algorithm_hash<hc3_iter, ltj::veo::veo_adaptive<hc3_iter, ltj::util::trait_distinct>>;

    if (system == "xcltj")
        run<x_index, x_algo>(index, queries, timeout, threshold, out, skip, opt);
    else if (system == "xcltj-rb")
        run<xrb_index, xrb_algo>(index, queries, timeout, threshold, out, skip, opt);
    else if (system == "xcltj-rb-wg")
        run<xwg_index, xwg_algo>(index, queries, timeout, threshold, out, skip, opt);
    else if (system == "hcltj-rb")
        run<hrb_index, hrb_algo>(index, queries, timeout, threshold, out, skip, opt);
    else if (system == "hcltj-c3")
        run<hc3_index, hc3_algo>(index, queries, timeout, threshold, out, skip, opt);
    else
        run<h_index, h_algo>(index, queries, timeout, threshold, out, skip, opt);
    return 0;
}
