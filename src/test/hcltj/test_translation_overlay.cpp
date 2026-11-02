// Dual-phase checks of the translation overlay (L_plain and the policy against m_seq) on
// datasets that force the edge cases, plus move, swap, serialization and old-format rejection.
#include <index/cltj_index_metatrie.hpp>
#include <metatrie/overlay/checks.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using index_type = cltj::compact_ltj_metatrie_hash;
using trie_type = index_type::trie_type;
using cltj::overlay::CheckReport;

namespace {

int failures = 0;

void expect(bool cond, const std::string& what) {
    if (!cond) {
        std::cout << "  FAIL: " << what << "\n";
        failures++;
    }
}

index_type build_index(std::vector<cltj::spo_triple> triples, uint32_t threshold) {
    index_type index(triples);
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

struct Shape {
    uint64_t mphfs = 0;
    bool root_hashed = false;
    bool last_hashed = false;
    uint64_t fallbacks = 0;
};

Shape shape_of(const trie_type& trie) {
    Shape s;
    s.mphfs = trie.mphf_count();
    s.root_hashed = trie.node_has_hash(0);
    uint64_t last = 0;
    for (uint64_t v = 0, deg = 0; v + 1 < trie.louds_size(); v += deg) {
        deg = trie.children(v);
        last = v;
        if (trie.node_has_hash(v) && trie.hash_has_fallback(v))
            s.fallbacks++;
    }
    s.last_hashed = trie.node_has_hash(last);
    return s;
}

std::vector<Shape> check_index(const index_type& index, const std::string& label) {
    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
    std::vector<Shape> shapes;
    std::cout << label << "\n";
    for (int i = 0; i < 6; i++) {
        const auto& trie = index.tries[i];
        CheckReport dual = cltj::overlay::check_dual_phase(trie);
        CheckReport policy = cltj::overlay::check_policy(trie);
        Shape s = shape_of(trie);
        shapes.push_back(s);
        std::cout << "  " << names[i] << ": mphfs=" << s.mphfs << " root_hashed=" << s.root_hashed
                  << " last_hashed=" << s.last_hashed << " fallbacks=" << s.fallbacks << "\n";
        if (!dual.ok() || !policy.ok()) {
            cltj::overlay::print_report(std::cout, std::string(names[i]) + " dual", dual);
            cltj::overlay::print_report(std::cout, std::string(names[i]) + " policy", policy);
            failures++;
        }
        expect(dual.comparisons > 0 && policy.comparisons > 0, std::string(names[i]) + ": checks ran");
    }
    return shapes;
}

std::vector<cltj::spo_triple> grid(uint32_t ns, uint32_t np, uint32_t no) {
    std::vector<cltj::spo_triple> d;
    for (uint32_t s = 1; s <= ns; s++)
        for (uint32_t p = 1; p <= np; p++)
            for (uint32_t o = 1; o <= no; o++)
                d.push_back({s, p, o});
    return d;
}

std::vector<cltj::spo_triple> random_triples(uint32_t seed, uint32_t n, uint32_t range) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<uint32_t> dist(1, range);
    std::vector<cltj::spo_triple> d;
    for (uint32_t i = 0; i < n; i++)
        d.push_back({dist(rng), dist(rng), dist(rng)});
    return d;
}

std::vector<cltj::spo_triple> load_dataset(const std::string& path) {
    std::ifstream ifs(path);
    std::vector<cltj::spo_triple> d;
    uint32_t s, p, o;
    while (ifs >> s >> p >> o)
        d.push_back({s, p, o});
    return d;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dataset_path = argc > 1 ? argv[1] : "data/data.txt";

    auto data = load_dataset(dataset_path);
    expect(!data.empty(), "dataset " + dataset_path + " loaded");
    if (!data.empty())
        for (uint32_t t : {2u, 3u, 4u, 1000u})
            check_index(build_index(data, t), dataset_path + ", t=" + std::to_string(t));

    // Root hashed, last node hashed, partial trie with hashed nodes (its node 0 included).
    {
        auto shapes = check_index(build_index(grid(5, 2, 6), 3), "grid 5x2x6, t=3");
        expect(shapes[0].root_hashed, "SPO root hashed");
        expect(shapes[0].last_hashed, "SPO last node hashed");
        expect(shapes[1].mphfs > 0 && shapes[1].root_hashed, "SOP (partial) hashed, node 0 included");
    }

    // No MPHF anywhere: every trie takes the early return of reorder_louds_by_mphf().
    {
        auto shapes = check_index(build_index(grid(5, 2, 6), 1000), "grid 5x2x6, t=1000");
        for (int i = 0; i < 6; i++)
            expect(shapes[i].mphfs == 0, "trie " + std::to_string(i) + " has no MPHF");
    }

    // Partial trie without MPHFs that still goes through the reorder loop (root permutation).
    {
        auto shapes = check_index(build_index(grid(5, 1, 2), 3), "grid 5x1x2, t=3");
        expect(shapes[0].root_hashed, "SPO root hashed");
        expect(shapes[1].mphfs == 0, "SOP has no MPHF");
    }

    // Fallback: with t=2 there are hundreds of tiny key sets, and some fail to peel often
    // enough that their MPHF keeps residual keys.
    {
        uint64_t fallbacks = 0;
        for (uint32_t seed = 1; seed <= 20 && fallbacks == 0; seed++) {
            auto shapes = check_index(
                build_index(random_triples(seed, 3000, 60), 2),
                "random seed=" + std::to_string(seed) + " n=3000 range=60, t=2 (fallback)"
            );
            for (auto& s : shapes)
                fallbacks += s.fallbacks;
        }
        expect(fallbacks > 0, "some hashed node uses the fallback");
    }

    for (uint32_t seed = 1; seed <= 6; seed++)
        for (uint32_t t : {2u, 5u, 20u, 100u})
            check_index(
                build_index(random_triples(seed, 3000, 60), t),
                "random seed=" + std::to_string(seed) + " n=3000 range=60, t=" + std::to_string(t)
            );

    // Move, swap and the serialization round trip keep every support bound to its own B.
    {
        index_type a = build_index(grid(5, 2, 6), 3);
        index_type b = build_index(random_triples(7, 3000, 60), 5);
        a.get_trie(0)->swap(*b.get_trie(0));
        check_index(a, "after swap (a)");
        check_index(b, "after swap (b)");
        a.get_trie(0)->swap(*b.get_trie(0));

        index_type moved(std::move(b));
        check_index(moved, "moved");

        std::stringstream ss;
        moved.serialize(ss);
        index_type loaded;
        loaded.load(ss);
        check_index(loaded, "loaded");
        for (int i = 0; i < 6; i++) {
            expect(
                loaded.tries[i].seq_plain() == moved.tries[i].seq_plain(),
                "trie " + std::to_string(i) + ": L_plain survives the round trip"
            );
            expect(
                sdsl::size_in_bytes(loaded.tries[i]) == sdsl::size_in_bytes(moved.tries[i]),
                "trie " + std::to_string(i) + ": same serialized size after the round trip"
            );
        }
    }

    // A pre-overlay trie starts with the bit size of B; a wrong version byte is rejected too.
    {
        auto expect_rejected = [](std::stringstream& ss, const std::string& what) {
            trie_type trie;
            try {
                trie.load(ss);
                expect(false, what + " is rejected");
            } catch (const std::runtime_error& e) {
                std::cout << "  rejected (" << what << "): " << e.what() << "\n";
            }
        };
        std::stringstream old_format;
        sdsl::bit_vector(1000, 1).serialize(old_format);
        expect_rejected(old_format, "old format");

        std::stringstream wrong_version;
        uint64_t tag = trie_type::format_tag ^ 0xFF;
        sdsl::write_member(tag, wrong_version);
        expect_rejected(wrong_version, "wrong version");
    }

    std::cout << (failures == 0 ? "ALL PASS" : "FAILURES: " + std::to_string(failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
