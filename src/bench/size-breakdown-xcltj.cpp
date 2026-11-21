// Serialized bytes of a static xCLTJ (or xCLTJ-EF) index, member by member. The file is read
// in the order of load(), measuring each member by its position in the stream, so the members
// add up to the file by construction; the per-trie and total sizes are checked against
// sdsl::size_in_bytes of the loaded index. Output: one line per trie, one per Elias-Fano class
// (EF only), one for the root bitmaps, the totals per member, and the sdsl structure tree as JSON
// if a second argument is given. Exits with 1 if anything does not add up.
// Usage: size-breakdown-xcltj <index> [out.json]
#include <index/cltj_index_metatrie.hpp>
#include <sdsl/io.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#ifdef CLTJ_EF
typedef cltj::compact_ltj_metatrie_ef index_type;
#else
typedef cltj::compact_ltj_metatrie index_type;
#endif

typedef std::vector<std::pair<std::string, uint64_t>> members_type;

static const char *names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

struct reader {
    std::ifstream in;
    uint64_t pos = 0;

    template<class T>
    uint64_t load(T &x) {
        x.load(in);
        return advance();
    }

    template<class T>
    uint64_t load(T &x, const sdsl::bit_vector *v) {
        x.load(in, v);
        return advance();
    }

    uint64_t read(uint64_t &x) {
        sdsl::read_member(x, in);
        return advance();
    }

    uint64_t advance() {
        uint64_t now = in.tellg();
        uint64_t bytes = now - pos;
        pos = now;
        return bytes;
    }
};

static uint64_t print_members(const members_type &m, std::map<std::string, uint64_t> &totals) {
    uint64_t sum = 0;
    for (const auto &x: m) {
        std::cout << " " << x.first << "=" << x.second;
        totals[x.first] += x.second;
        sum += x.second;
    }
    std::cout << " sum=" << sum;
    return sum;
}

// Edges of level 2: children of the first n1 nodes. B has one bit per label plus a leading 0,
// with a 0 at the last child of every node, so the (n1 + 1)-th 0 is at the last level-2 label.
static uint64_t level2_edges(const sdsl::select_support_mcl<0> &select0, uint64_t n1) {
    return select0.select(n1 + 1);
}

#ifdef CLTJ_EF
static uint64_t trie_breakdown(reader &r, int t, uint64_t n1, uint64_t bytes, std::map<std::string, uint64_t> &totals,
                               std::vector<std::string> &class_lines) {
    sdsl::bit_vector bv;
    cds::succ_support_v<0> succ0;
    sdsl::select_support_mcl<0> select0;
    sdsl::int_vector<> seq2s, seq3s;
    sdsl::int_vector<64> kd, hdr;
    uint64_t n2s = 0, nlong = 0, ncls = 0;
    members_type m;
    m.push_back({"bv", r.load(bv)});
    m.push_back({"succ0", r.load(succ0, &bv)});
    m.push_back({"select0", r.load(select0, &bv)});
    m.push_back({"seq2_short", r.load(seq2s)});
    m.push_back({"seq3_short", r.load(seq3s)});
    m.push_back({"n2_short", r.read(n2s)});
    m.push_back({"KD", r.load(kd)});
    m.push_back({"nlong", r.read(nlong)});
    m.push_back({"long_headers", r.load(hdr)});
    m.push_back({"nclasses", r.read(ncls)});

    const uint64_t n2 = level2_edges(select0, n1);
    // header record of a long node: start | class << 32, length, ... (8 words)
    std::vector<uint64_t> cls_nodes(ncls, 0), cls_labels(ncls, 0), cls_level(ncls, 0);
    uint64_t long2 = 0, long3 = 0;
    for (uint64_t k = 0; k < nlong; ++k) {
        const uint64_t start = hdr[8 * k] & 0xffffffffULL, cls = hdr[8 * k] >> 32, len = hdr[8 * k + 1];
        ++cls_nodes[cls];
        cls_labels[cls] += len;
        cls_level[cls] = start < n2 ? 2 : 3;
        (start < n2 ? long2 : long3) += len;
    }
    uint64_t ef_bytes = 0;
    for (uint64_t c = 0; c < ncls; ++c) {
        sdsl::sd_vector<> sd;
        const uint64_t sd_bytes = r.load(sd);
        ef_bytes += sd_bytes;
        const uint64_t low = sdsl::size_in_bytes(sd.low), high = sdsl::size_in_bytes(sd.high);
        const uint64_t sel1 = sdsl::size_in_bytes(sd.high_1_select), sel0 = sdsl::size_in_bytes(sd.high_0_select);
        class_lines.push_back("class trie=" + std::to_string(t) + " name=" + names[t] + " class=" + std::to_string(c)
                              + " level=" + std::to_string(cls_level[c]) + " nodes=" + std::to_string(cls_nodes[c])
                              + " labels=" + std::to_string(cls_labels[c]) + " ones=" + std::to_string(sd.low.size())
                              + " universe=" + std::to_string(sd.size()) + " wl=" + std::to_string(int(sd.wl))
                              + " bytes=" + std::to_string(sd_bytes) + " low=" + std::to_string(low)
                              + " high=" + std::to_string(high) + " high_1_select=" + std::to_string(sel1)
                              + " high_0_select=" + std::to_string(sel0) + " size_and_wl="
                              + std::to_string(sd_bytes - low - high - sel1 - sel0));
    }
    m.push_back({"ef_classes", ef_bytes});

    std::cout << "trie=" << t << " name=" << names[t] << " bytes=" << bytes << " louds_size=" << bv.size() << " n1=" << n1
              << " short2_size=" << seq2s.size() << " short2_width=" << int(seq2s.width())
              << " short3_size=" << seq3s.size() << " short3_width=" << int(seq3s.width())
              << " long_nodes=" << nlong << " long2_labels=" << long2 << " long3_labels=" << long3
              << " classes=" << ncls;
    const uint64_t sum = print_members(m, totals);
    std::cout << std::endl;
    if (n2s != seq2s.size() || n2 != seq2s.size() + long2) {
        std::cerr << "ERROR: trie " << t << ": n2_short=" << n2s << " seq2_short=" << seq2s.size() << " long2="
                  << long2 << " level-2 edges in B=" << n2 << std::endl;
        return ~0ULL;
    }
    return sum;
}
#else
static uint64_t trie_breakdown(reader &r, int t, uint64_t n1, uint64_t bytes, std::map<std::string, uint64_t> &totals,
                               std::vector<std::string> &) {
    sdsl::bit_vector bv;
    sdsl::int_vector<> seq2, seq3;
    uint64_t n2 = 0;
    cds::succ_support_v<0> succ0;
    sdsl::select_support_mcl<0> select0;
    members_type m;
    m.push_back({"bv", r.load(bv)});
    m.push_back({"seq2", r.load(seq2)});
    m.push_back({"seq3", r.load(seq3)});
    m.push_back({"n2", r.read(n2)});
    m.push_back({"succ0", r.load(succ0, &bv)});
    m.push_back({"select0", r.load(select0, &bv)});

    std::cout << "trie=" << t << " name=" << names[t] << " bytes=" << bytes << " louds_size=" << bv.size() << " n1=" << n1
              << " seq2_size=" << seq2.size() << " seq2_width=" << int(seq2.width())
              << " seq3_size=" << seq3.size() << " seq3_width=" << int(seq3.width());
    const uint64_t sum = print_members(m, totals);
    std::cout << std::endl;
    if (n2 != seq2.size() || n2 != level2_edges(select0, n1)) {
        std::cerr << "ERROR: trie " << t << ": n2=" << n2 << " seq2=" << seq2.size() << " level-2 edges in B="
                  << level2_edges(select0, n1) << std::endl;
        return ~0ULL;
    }
    return sum;
}
#endif

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        std::cout << "Usage: " << argv[0] << " <index> [out.json]" << std::endl;
        return 0;
    }
    std::string file = argv[1];

    index_type index;
    if (!sdsl::load_from_file(index, file)) {
        std::cerr << "ERROR: cannot load " << file << std::endl;
        return 2;
    }
    const uint64_t index_bytes = sdsl::size_in_bytes(index);

    reader r;
    r.in.open(file, std::ios::binary);
    r.in.seekg(0, std::ios::end);
    const uint64_t file_bytes = r.in.tellg();
    r.in.seekg(0);

    bool ok = true;
    std::map<std::string, uint64_t> totals;
    std::vector<std::string> class_lines;
    uint64_t sum_tries = 0;
    for (int t = 0; t < 6; ++t) {
        const uint64_t trie_bytes = sdsl::size_in_bytes(index.tries[t]);
        const uint64_t before = r.pos;
        const uint64_t sum = trie_breakdown(r, t, index.gaps[t / 2], trie_bytes, totals, class_lines);
        if (sum == ~0ULL || sum != trie_bytes || r.pos - before != trie_bytes) {
            std::cerr << "ERROR: trie " << t << ": members add up to " << sum << " of " << trie_bytes << " bytes"
                      << std::endl;
            ok = false;
        }
        sum_tries += trie_bytes;
    }
    for (const auto &l: class_lines) std::cout << l << std::endl;

    uint64_t gap, gaps_bytes = 0;
    for (int i = 0; i < 3; ++i) gaps_bytes += r.read(gap);
    std::cout << "gaps bytes=" << gaps_bytes << " s=" << index.gaps[0] << " p=" << index.gaps[1]
              << " o=" << index.gaps[2] << std::endl;

    sdsl::bit_vector bs, bo, bp;
    cds::succ_support_v<1> ss, so, sp;
    sdsl::rank_support_v<1> rs, ro, rp;
    members_type root;
    root.push_back({"s.bv", r.load(bs)});
    root.push_back({"o.bv", r.load(bo)});
    root.push_back({"p.bv", r.load(bp)});
    root.push_back({"s.succ", r.load(ss, &bs)});
    root.push_back({"o.succ", r.load(so, &bo)});
    root.push_back({"p.succ", r.load(sp, &bp)});
    root.push_back({"s.rank", r.load(rs, &bs)});
    root.push_back({"o.rank", r.load(ro, &bo)});
    root.push_back({"p.rank", r.load(rp, &bp)});
    uint64_t root_bytes = 0;
    for (const auto &x: root) root_bytes += x.second;
    std::cout << "root bytes=" << root_bytes;
    std::map<std::string, uint64_t> root_totals;
    print_members(root, root_totals);
    std::cout << " s.universe=" << bs.size() << " s.count=" << rs(bs.size()) << " p.universe=" << bp.size()
              << " p.count=" << rp(bp.size()) << " o.universe=" << bo.size() << " o.count=" << ro(bo.size())
              << std::endl;

    std::cout << "totals";
    for (const auto &x: totals) std::cout << " " << x.first << "=" << x.second;
    std::cout << " tries=" << sum_tries << " gaps=" << gaps_bytes << " root=" << root_bytes << std::endl;

    const uint64_t parsed = sum_tries + gaps_bytes + root_bytes;
    std::cout << "index_bytes=" << index_bytes << " file_bytes=" << file_bytes << " parsed_bytes=" << parsed
              << std::endl;
    if (parsed != index_bytes || parsed != file_bytes || r.pos != file_bytes) {
        std::cerr << "ERROR: tries, gaps and root add up to " << parsed << "; index " << index_bytes << ", file "
                  << file_bytes << ", read up to " << r.pos << std::endl;
        ok = false;
    }

    if (argc == 3) {
        std::ofstream out(argv[2]);
        sdsl::write_structure<sdsl::JSON_FORMAT>(index, out);
        if (!out) {
            std::cerr << "ERROR: cannot write " << argv[2] << std::endl;
            return 2;
        }
    }
    std::cout << (ok ? "RESULT: SIZES ADD UP" : "RESULT: FAIL") << std::endl;
    return ok ? 0 : 1;
}
