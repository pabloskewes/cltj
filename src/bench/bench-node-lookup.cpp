// Cost of one lookup or seek inside one trie node, per structure, from the
// sorted labels of a real node (a binary file of uint32, as written by
// extract-node-keys).
//
// Regimes:
//   hot   one instance; point lookups cycle over a working set of --hot-set
//         keys, seeks visit the same instance again and again.
//   cold  --cold-bytes worth of copies of the structure; every point lookup,
//         and every visit of a seek sweep, goes to a random copy.
// Modes:
//   indep  queries do not depend on each other (throughput).
//   chain  the index of the next query depends on the answer of the previous one.
//   fence  chain plus a serializing fence between queries (full latency).
// Seek sweeps visit the node from the start with targets every --gaps
// positions (from a random offset), each seek starting at the answer of the
// previous one, as a leap of LTJ does. The MPHF answers the same targets with
// lookups, since it has no order.

#include <bench/node_lookup.hpp>

#include <CLI11.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#ifdef __linux__
#include <sys/mman.h>
#ifndef MADV_COLLAPSE
#define MADV_COLLAPSE 25  // linux/mman.h, kernel 6.1; glibc < 2.37 does not define it
#endif
#endif

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
static inline void serialize_fence() { _mm_lfence(); }
#else
static inline void serialize_fence() { asm volatile("dsb ish\n\tisb" ::: "memory"); }
#endif

using namespace cltj::node_lookup;

namespace {

enum Mode { INDEP = 0, CHAIN = 1, FENCE = 2 };
const char* mode_name[] = {"indep", "chain", "fence"};

struct Query {
    uint32_t key;
    uint32_t copy;
    uint32_t start;  // 1 at the first seek of a visit
};

struct Config {
    std::string node;
    std::vector<uint32_t> keys;
    uint8_t width = 32;
    uint64_t queries = 1 << 20;
    uint32_t reps = 5;
    uint64_t hot_set = 4096;
    uint64_t cold_bytes = 1ULL << 30;
    uint64_t max_copies = 1 << 17;
    std::vector<uint64_t> gaps;
    std::vector<std::string> structures;
    std::vector<int> modes;
    bool point = true, seek = true;
    uint64_t seed = 42;
};

std::vector<uint32_t> read_keys(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        throw std::runtime_error("cannot open " + path);
    size_t bytes = in.tellg();
    in.seekg(0);
    std::vector<uint32_t> keys(bytes / sizeof(uint32_t));
    in.read(reinterpret_cast<char*>(keys.data()), keys.size() * sizeof(uint32_t));
    for (size_t i = 1; i < keys.size(); ++i)
        if (keys[i] <= keys[i - 1])
            throw std::runtime_error("keys not strictly increasing in " + path);
    return keys;
}

uint64_t lower_pos(const std::vector<uint32_t>& keys, uint32_t c) {
    return std::lower_bound(keys.begin(), keys.end(), c) - keys.begin();
}

bool member(const std::vector<uint32_t>& keys, uint32_t c) {
    return std::binary_search(keys.begin(), keys.end(), c);
}

// A label in [first, last] that is not in the node.
uint32_t random_absent(const std::vector<uint32_t>& keys, std::mt19937_64& rng) {
    std::uniform_int_distribution<uint64_t> d(keys.front(), keys.back());
    while (true) {
        uint32_t c = d(rng);
        if (!member(keys, c))
            return c;
    }
}

std::vector<Query> point_queries(const Config& cfg, bool hot, bool present, uint64_t copies, uint64_t seed) {
    const auto& keys = cfg.keys;
    std::mt19937_64 rng(seed);
    std::vector<uint32_t> pool;
    if (hot) {
        uint64_t w = std::min<uint64_t>(cfg.hot_set, keys.size());
        for (uint64_t i = 0; i < w; ++i)
            pool.push_back(present ? keys[rng() % keys.size()] : random_absent(keys, rng));
    }
    std::vector<Query> qs(cfg.queries);
    for (auto& q : qs) {
        if (hot)
            q.key = pool[rng() % pool.size()];
        else
            q.key = present ? keys[rng() % keys.size()] : random_absent(keys, rng);
        q.copy = hot ? 0 : rng() % copies;
        q.start = 1;
    }
    return qs;
}

// Targets of seek sweeps with gap g. Returns the fraction of targets that are absent.
double seek_queries(const Config& cfg, uint64_t g, bool hot, bool present, uint64_t copies, uint64_t seed,
                    std::vector<Query>& qs) {
    const auto& keys = cfg.keys;
    const uint64_t n = keys.size();
    std::mt19937_64 rng(seed);
    qs.clear();
    qs.reserve(cfg.queries);
    uint64_t absent = 0;
    while (qs.size() < cfg.queries) {
        uint64_t o = rng() % g;
        uint32_t copy = hot ? 0 : rng() % copies;
        uint64_t prev = 0;
        bool first = true;
        for (uint64_t p = o; p < n && qs.size() < cfg.queries; p += g) {
            Query q{keys[p], copy, first ? 1u : 0u};
            if (!present) {
                uint64_t lo = first ? 1 : prev + 1;
                for (uint64_t j = p; j >= lo && j >= 1; --j) {
                    if (keys[j] - keys[j - 1] > 1) {
                        q.key = keys[j] - 1;
                        ++absent;
                        break;
                    }
                }
            }
            prev = p;
            first = false;
            qs.push_back(q);
        }
    }
    return double(absent) / qs.size();
}

template <class S>
inline Answer do_lookup(const S& s, const Query& q, uint64_t&, uint64_t&) {
    return s.lookup(q.key);
}

template <class S>
inline Answer do_seek(const S& s, const Query& q, uint64_t& a, uint64_t& cur) {
    if (q.start) {
        a = 0;
        cur = no_cursor;
    }
    Answer r = s.seek(q.key, a, cur);
    a = r.pos;
    cur = r.cur;
    return r;
}

template <class S, bool Seek, int M>
uint64_t run_loop(const std::vector<S>& copies, const std::vector<Query>& qs, uint64_t count, uint64_t zero) {
    uint64_t acc = 0, a = 0, cur = no_cursor;
    uint64_t i = 0;
    const uint64_t n = qs.size();
    for (uint64_t k = 0; k < count; ++k) {
        const Query& q = qs[i];
        Answer r;
        if constexpr (Seek)
            r = do_seek(copies[q.copy], q, a, cur);
        else
            r = do_lookup(copies[q.copy], q, a, cur);
        uint64_t res = r.pos ^ r.found;
        acc += res;
        uint64_t next = k + 1;
        if (M != INDEP)
            next += res & zero;
        if (M == FENCE)
            serialize_fence();
        i = next < n ? next : next - n;
    }
    return acc;
}

template <class S, bool Seek>
uint64_t run_mode(int mode, const std::vector<S>& copies, const std::vector<Query>& qs, uint64_t count,
                  uint64_t zero) {
    switch (mode) {
        case INDEP: return run_loop<S, Seek, INDEP>(copies, qs, count, zero);
        case CHAIN: return run_loop<S, Seek, CHAIN>(copies, qs, count, zero);
        default: return run_loop<S, Seek, FENCE>(copies, qs, count, zero);
    }
}

// The MPHF answers slots, not positions: only membership is checked against the oracle.
template <class S>
constexpr bool has_order() {
    return !std::is_same<S, GlGhNode>::value && !std::is_same<S, PackedGlGhNode>::value;
}

template <class S, bool Seek>
uint64_t verify(const std::vector<S>& copies, const std::vector<Query>& qs, const Config& cfg) {
    uint64_t a = 0, cur = no_cursor, bad = 0;
    for (const auto& q : qs) {
        Answer r;
        if constexpr (Seek)
            r = do_seek(copies[q.copy], q, a, cur);
        else
            r = do_lookup(copies[q.copy], q, a, cur);
        bool ok = r.found == member(cfg.keys, q.key);
        if constexpr (has_order<S>()) {
            uint64_t want_pos = lower_pos(cfg.keys, q.key);
            ok = ok && r.pos == want_pos && r.value == cfg.keys[want_pos];
        } else {
            ok = ok && (!r.found || r.pos < cfg.keys.size());
        }
        if (!ok)
            ++bad;
    }
    return bad;
}

struct Out {
    std::ofstream csv;
};

template <class S>
void bench_structure(const std::string& name, std::vector<S>& copies, const Config& cfg, Out& out,
                     uint64_t zero) {
    struct Case {
        bool seek;
        uint64_t gap;
        bool hot;
        bool present;
        int mode;
    };
    std::vector<Case> cases;
    for (int hot = 1; hot >= 0; --hot)
        for (int present = 1; present >= 0; --present)
            for (int m : cfg.modes) {
                if (cfg.point)
                    cases.push_back({false, 0, bool(hot), bool(present), m});
                if (cfg.seek)
                    for (uint64_t g : cfg.gaps)
                        if (g <= cfg.keys.size())
                            cases.push_back({true, g, bool(hot), bool(present), m});
            }

    const uint64_t ncopies = copies.size();
    for (uint32_t rep = 0; rep < cfg.reps; ++rep) {
        for (const auto& c : cases) {
            uint64_t seed = cfg.seed ^ (uint64_t(c.seek) << 60) ^ (c.gap << 20) ^ (uint64_t(c.hot) << 58) ^
                (uint64_t(c.present) << 57) ^ (uint64_t(rep) << 40);
            std::vector<Query> qs;
            double absent_frac = c.present ? 0.0 : 1.0;
            if (c.seek)
                absent_frac = seek_queries(cfg, c.gap, c.hot, c.present, ncopies, seed, qs);
            else
                qs = point_queries(cfg, c.hot, c.present, ncopies, seed);

            constexpr bool ordered_seek = has_order<S>();
            auto timed = [&](int mode, uint64_t count) {
                if constexpr (ordered_seek) {
                    if (c.seek)
                        return run_mode<S, true>(mode, copies, qs, count, zero);
                }
                return run_mode<S, false>(mode, copies, qs, count, zero);
            };
            if (rep == 0) {
                uint64_t bad = 0;
                if constexpr (ordered_seek)
                    bad = c.seek ? verify<S, true>(copies, qs, cfg) : verify<S, false>(copies, qs, cfg);
                else
                    bad = verify<S, false>(copies, qs, cfg);
                if (bad) {
                    std::cerr << "VERIFY FAILED " << name << " seek=" << c.seek << " gap=" << c.gap
                              << " hot=" << c.hot << " present=" << c.present << " bad=" << bad << std::endl;
                    std::exit(2);
                }
            } else {
                timed(INDEP, std::min<uint64_t>(qs.size(), std::max<uint64_t>(cfg.hot_set, 65536)));
            }

            auto t0 = std::chrono::steady_clock::now();
            uint64_t acc = timed(c.mode, qs.size());
            auto t1 = std::chrono::steady_clock::now();
            double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / qs.size();

            out.csv << cfg.node << ',' << cfg.keys.size() << ',' << name << ',' << (c.seek ? "seek" : "lookup")
                    << ',' << c.gap << ',' << (c.hot ? "hot" : "cold") << ',' << (c.present ? "present" : "absent")
                    << ',' << mode_name[c.mode] << ',' << qs.size() << ',' << rep << ',' << ns << ','
                    << absent_frac << ',' << ncopies << ',' << copies[0].size_in_bytes() << ',' << acc << '\n';
            out.csv.flush();
        }
        std::cerr << "[" << cfg.node << "] " << name << " rep " << rep << " done" << std::endl;
    }
}

template <class S>
std::vector<S> make_copies(S&& base, const Config& cfg) {
    uint64_t bytes = std::max<uint64_t>(base.size_in_bytes(), 1);
    uint64_t r = std::max<uint64_t>(1, (cfg.cold_bytes + bytes - 1) / bytes);
    r = std::min<uint64_t>(r, cfg.max_copies);
    std::vector<S> copies;
    copies.reserve(r);
    copies.push_back(std::move(base));
    for (uint64_t i = 1; i < r; ++i)
        copies.push_back(copies[0].clone());
    return copies;
}

// Backs every anonymous writable mapping of the process with huge pages
// (MADV_COLLAPSE, Linux >= 6.1), so that the copies are not limited by the
// TLB reach of 4 KB pages. Returns the bytes asked to collapse and how many of
// them failed.
std::pair<uint64_t, uint64_t> collapse_to_huge_pages() {
    uint64_t asked = 0, failed = 0;
#if defined(__linux__) && defined(MADV_COLLAPSE)
    const uint64_t huge = 2ULL << 20;
    std::ifstream in("/proc/self/maps");
    std::string line;
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string range, perms, offset, dev, inode, path;
        ls >> range >> perms >> offset >> dev >> inode;
        std::getline(ls, path);
        path.erase(0, path.find_first_not_of(' '));
        if (perms != "rw-p" || !(path.empty() || path == "[heap]"))
            continue;
        uint64_t beg = std::stoull(range.substr(0, range.find('-')), nullptr, 16);
        uint64_t end = std::stoull(range.substr(range.find('-') + 1), nullptr, 16);
        beg = (beg + huge - 1) & ~(huge - 1);
        end &= ~(huge - 1);
        if (end > beg)
            ranges.emplace_back(beg, end);
    }
    for (auto& r : ranges) {
        for (uint64_t b = r.first; b < r.second; b += huge) {
            asked += huge;
            if (madvise(reinterpret_cast<void*>(b), huge, MADV_COLLAPSE) != 0)
                failed += huge;
        }
    }
#endif
    return {asked, failed};
}

bool g_collapse = true;

void print_memory() {
    if (g_collapse) {
        auto t0 = std::chrono::steady_clock::now();
        auto r = collapse_to_huge_pages();
        auto t1 = std::chrono::steady_clock::now();
        std::cerr << "[mem] collapse asked " << (r.first >> 20) << " MB, failed " << (r.second >> 20) << " MB, "
                  << std::chrono::duration<double>(t1 - t0).count() << " s" << std::endl;
    }
    std::ifstream in("/proc/self/smaps_rollup");
    std::string line;
    while (std::getline(in, line))
        if (line.rfind("Rss:", 0) == 0 || line.rfind("AnonHugePages:", 0) == 0)
            std::cerr << "[mem] " << line << std::endl;
}

std::vector<uint64_t> parse_list(const std::string& s) {
    std::vector<uint64_t> v;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ','))
        if (!tok.empty())
            v.push_back(std::stoull(tok));
    return v;
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Per-node lookup and seek microbenchmark: MPHF (GlGh, PackedGlGh) against EF and galloping"};
    Config cfg;
    std::string keys_path, out_path, space_path, gaps = "1,4,16,64,256,1024,4096,16384,65536";
    std::string structures = "glgh,packed,ef,gallop", modes = "indep,chain,fence";
    bool no_point = false, no_seek = false;
    std::string pages = "huge";
    int width = 0;
    app.add_option("keys", keys_path, "Sorted labels of the node (binary uint32)")->required();
    app.add_option("-o,--out", out_path, "Timing CSV")->required();
    app.add_option("--space", space_path, "Space CSV")->required();
    app.add_option("--node", cfg.node, "Node name for the CSV")->required();
    app.add_option("--width", width, "Label width of X for this level")->required();
    app.add_option("--queries", cfg.queries, "Queries per measurement");
    app.add_option("--reps", cfg.reps, "Repetitions (rep 0 also verifies against the oracle)");
    app.add_option("--hot-set", cfg.hot_set, "Distinct keys of a hot point-lookup working set");
    app.add_option("--cold-bytes", cfg.cold_bytes, "Bytes of copies for the cold regime");
    app.add_option("--max-copies", cfg.max_copies, "Upper bound on the number of copies");
    app.add_option("--gaps", gaps, "Seek gaps, comma separated");
    app.add_option("--structures", structures, "Subset of glgh,packed,ef,gallop");
    app.add_option("--modes", modes, "Subset of indep,chain,fence");
    app.add_option("--seed", cfg.seed, "Seed");
    app.add_option("--pages", pages, "huge: collapse the structures to huge pages before measuring; default: leave them")
        ->check(CLI::IsMember({"huge", "default"}));
    app.add_flag("--no-point", no_point, "Skip point lookups");
    app.add_flag("--no-seek", no_seek, "Skip seek sweeps");
    CLI11_PARSE(app, argc, argv);

    cfg.width = static_cast<uint8_t>(width);
    g_collapse = pages == "huge";
    cfg.point = !no_point;
    cfg.seek = !no_seek;
    cfg.gaps = parse_list(gaps);
    {
        std::stringstream ss(structures);
        std::string t;
        while (std::getline(ss, t, ','))
            cfg.structures.push_back(t);
        std::stringstream ms(modes);
        while (std::getline(ms, t, ',')) {
            if (t == "indep") cfg.modes.push_back(INDEP);
            else if (t == "chain") cfg.modes.push_back(CHAIN);
            else if (t == "fence") cfg.modes.push_back(FENCE);
            else throw std::runtime_error("unknown mode " + t);
        }
    }
    cfg.keys = read_keys(keys_path);
    std::cerr << "[" << cfg.node << "] n=" << cfg.keys.size() << " first=" << cfg.keys.front()
              << " last=" << cfg.keys.back() << std::endl;

    volatile uint64_t zero_v = 0;
    const uint64_t zero = zero_v;

    Out out;
    out.csv.open(out_path, std::ios::app);
    std::ofstream space(space_path, std::ios::app);
    out.csv.precision(6);
    space.precision(6);

    auto t_build = [](auto&& f) {
        auto t0 = std::chrono::steady_clock::now();
        auto r = f();
        auto t1 = std::chrono::steady_clock::now();
        return std::make_pair(std::move(r), std::chrono::duration<double>(t1 - t0).count());
    };
    const uint64_t n = cfg.keys.size();
    const uint64_t u = uint64_t(cfg.keys.back()) - cfg.keys.front() + 1;

    for (const auto& s : cfg.structures) {
        if (s == "glgh" || s == "packed") {
            auto run = [&](auto tag) {
                using H = decltype(tag);
                auto built = t_build([&] { return H(cfg.keys); });
                auto b = built.first.breakdown();
                space << cfg.node << ',' << n << ',' << u << ',' << s << ',' << built.first.size_in_bytes() << ','
                      << 8.0 * built.first.size_in_bytes() / n << ',' << b.g_bytes << ',' << b.used_pos_bytes << ','
                      << b.rank_bytes << ',' << b.q_bytes << ',' << b.other_bytes << ',' << b.fallback_bytes << ','
                      << built.first.mphf().n_residual() << ',' << built.second << '\n';
                space.flush();
                auto copies = make_copies(std::move(built.first), cfg);
                std::cerr << "[" << cfg.node << "] " << s << " copies=" << copies.size() << std::endl;
                print_memory();
                bench_structure(s, copies, cfg, out, zero);
            };
            if (s == "glgh")
                run(GlGhNode());
            else
                run(PackedGlGhNode());
        } else if (s == "ef") {
            auto built = t_build([&] { return EFNode(cfg.keys); });
            space << cfg.node << ',' << n << ',' << u << ',' << s << ',' << built.first.size_in_bytes() << ','
                  << 8.0 * built.first.size_in_bytes() / n << ",,,,,,," << built.first.low_width() << ','
                  << built.second << '\n';
            space.flush();
            auto copies = make_copies(std::move(built.first), cfg);
            std::cerr << "[" << cfg.node << "] ef copies=" << copies.size() << std::endl;
            print_memory();
            bench_structure(s, copies, cfg, out, zero);
        } else if (s == "gallop") {
            auto built = t_build([&] { return GallopNode(cfg.keys, cfg.width); });
            space << cfg.node << ',' << n << ',' << u << ',' << s << ',' << built.first.size_in_bytes() << ','
                  << 8.0 * built.first.size_in_bytes() / n << ",,,,,,," << built.first.width() << ','
                  << built.second << '\n';
            space.flush();
            auto copies = make_copies(std::move(built.first), cfg);
            std::cerr << "[" << cfg.node << "] gallop copies=" << copies.size() << std::endl;
            print_memory();
            bench_structure(s, copies, cfg, out, zero);
        } else {
            throw std::runtime_error("unknown structure " + s);
        }
    }
    return 0;
}
