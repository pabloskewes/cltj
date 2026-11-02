// Scheme C (StepTableOverlay): edge cases of the lookup on hand-made tables, and a
// differential against scheme A on full builds of the same data.
#include <index/cltj_index_metatrie.hpp>
#include <metatrie/overlay/checks.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using cltj::overlay::CheckReport;
using cltj::overlay::SepBitmapOverlay;
using cltj::overlay::StepTableOverlay;
using index_a = cltj::cltj_index_metatrie<cltj::compact_metatrie_hash<SepBitmapOverlay>>;
using index_c = cltj::cltj_index_metatrie<cltj::compact_metatrie_hash<StepTableOverlay>>;

namespace {

int failures = 0;

void expect(bool cond, const std::string& what) {
    if (!cond) {
        std::cout << "  FAIL: " << what << "\n";
        failures++;
    }
}

struct Block {
    uint64_t pos, deg;
    bool hashed;
};

StepTableOverlay table_of(const std::vector<Block>& blocks) {
    StepTableOverlay ov;
    ov.begin(0, 0);
    for (const auto& b : blocks) {
        if (b.hashed)
            ov.on_hashed_node(b.pos, b.deg);
        else
            ov.on_plain_node(b.pos, b.deg);
    }
    ov.finalize(sdsl::bit_vector(), 0);
    return ov;
}

void expect_lookup(
    const StepTableOverlay& ov, uint64_t v, bool hashed, uint64_t idx, uint64_t start, const std::string& what
) {
    std::ostringstream got;
    got << what << " (v=" << v << "): has_dict=" << ov.has_dict(v) << " dict_idx=" << ov.dict_idx(v)
        << " plain_start=" << ov.plain_start(v) << ", expected " << hashed << " " << idx << " " << start;
    expect(ov.has_dict(v) == hashed && ov.dict_idx(v) == idx && ov.plain_start(v) == start, got.str());
}

void unit_cases() {
    std::cout << "unit cases\n";

    // hashed_start = [3, 13, 18], cum_deleted = [4, 9, 13]; the last block is hashed.
    {
        auto ov = table_of({
            {0,  3, false},
            {3,  4, true },
            {7,  3, false},
            {10, 3, false},
            {13, 5, true },
            {18, 4, true },
        });
        expect(ov.hashed_count() == 3, "three rows");
        expect_lookup(ov, 0, false, 0, 0, "before the first hashed node (r = 0)");
        expect_lookup(ov, 3, true, 0, 3, "first hashed node");
        expect_lookup(ov, 7, false, 1, 3, "after one hashed node");
        expect_lookup(ov, 10, false, 1, 6, "plain node between hashed ones");
        expect_lookup(ov, 13, true, 1, 9, "hashed node");
        expect_lookup(ov, 18, true, 2, 9, "hashed node right after a hashed one");
        expect_lookup(ov, 22, false, 3, 9, "sentinel after the last hashed node (r = |hashed_start|)");
    }

    // Root hashed, then plain blocks after the last hashed node.
    {
        auto ov = table_of({
            {0, 5, true },
            {5, 3, false},
            {8, 2, false},
        });
        expect_lookup(ov, 0, true, 0, 0, "hashed root");
        expect_lookup(ov, 5, false, 1, 0, "plain node after the last hashed (r = |hashed_start|)");
        expect_lookup(ov, 8, false, 1, 3, "second plain node after the last hashed");
        expect_lookup(ov, 10, false, 1, 5, "sentinel");
    }

    // No hashed node: the identity.
    {
        auto ov = table_of({
            {0, 2, false},
            {2, 4, false},
        });
        expect(ov.hashed_count() == 0, "no rows");
        expect_lookup(ov, 0, false, 0, 0, "no hashed nodes, v=0");
        expect_lookup(ov, 2, false, 0, 2, "no hashed nodes, v=2");
        expect_lookup(ov, 6, false, 0, 6, "no hashed nodes, sentinel");
    }

    // Serialization round trip and size breakdown.
    {
        auto ov = table_of({
            {0,  3, false},
            {3,  4, true },
            {7,  3, false},
            {13, 5, true },
        });
        std::stringstream ss;
        size_t written = ov.serialize(ss);
        StepTableOverlay loaded;
        loaded.load(ss);
        expect(written == ss.str().size(), "serialize reports the bytes it wrote");
        expect(loaded.hashed_count() == 2, "round trip keeps the rows");
        for (uint64_t v : {0, 3, 7, 10, 13, 18})
            expect(
                loaded.has_dict(v) == ov.has_dict(v) && loaded.dict_idx(v) == ov.dict_idx(v)
                    && loaded.plain_start(v) == ov.plain_start(v),
                "round trip keeps the lookup at v=" + std::to_string(v)
            );
        auto b = ov.get_size_breakdown();
        expect(b.components.size() == 2 && b.total_bytes() == written, "breakdown adds up to the serialized size");
    }

    // The build rejects positions or counts beyond 32 bits and non-increasing positions.
    {
        auto expect_throw = [](auto f, const std::string& what) {
            try {
                f();
                expect(false, what + " throws");
            } catch (const std::runtime_error& e) {
                std::cout << "  rejected (" << what << "): " << e.what() << "\n";
            }
        };
        expect_throw(
            [] {
                StepTableOverlay ov;
                ov.on_hashed_node(uint64_t(1) << 32, 4);
            },
            "position beyond 32 bits"
        );
        expect_throw(
            [] {
                StepTableOverlay ov;
                ov.on_hashed_node(0, 0xFFFFFFFFull);
                ov.on_hashed_node(0xFFFFFFFFull, 1);
            },
            "deleted count beyond 32 bits"
        );
        expect_throw(
            [] {
                StepTableOverlay ov;
                ov.on_hashed_node(10, 4);
                ov.on_hashed_node(10, 4);
            },
            "repeated position"
        );
    }
}

template <class Index>
Index build_index(std::vector<cltj::spo_triple> triples, uint32_t threshold) {
    Index index(triples);
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

// Structure and table consistency of C, and C against A, trie by trie.
void check_pair(const index_a& a, const index_c& c, const std::string& label) {
    static const char* names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
    std::cout << label << "\n";
    for (int i = 0; i < 6; i++) {
        const auto& ta = a.tries[i];
        const auto& tc = c.tries[i];
        CheckReport labels = cltj::overlay::check_labels(tc);
        CheckReport table = cltj::overlay::check_step_table(tc);
        CheckReport diff = cltj::overlay::check_overlay_equal(ta, tc);
        std::cout << "  " << names[i] << ": mphfs=" << tc.mphf_count() << " root_hashed=" << tc.node_has_hash(0)
                  << " compared=" << diff.comparisons << "\n";
        if (!labels.ok() || !table.ok() || !diff.ok()) {
            cltj::overlay::print_report(std::cout, std::string(names[i]) + " labels (C)", labels);
            cltj::overlay::print_report(std::cout, std::string(names[i]) + " table (C)", table);
            cltj::overlay::print_report(std::cout, std::string(names[i]) + " A vs C", diff);
            failures++;
        }
    }
}

void check_data(const std::vector<cltj::spo_triple>& data, uint32_t t, const std::string& label) {
    check_pair(build_index<index_a>(data, t), build_index<index_c>(data, t), label + ", t=" + std::to_string(t));
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

    unit_cases();

    auto data = load_dataset(dataset_path);
    expect(!data.empty(), "dataset " + dataset_path + " loaded");
    if (!data.empty())
        for (uint32_t t : {2u, 3u, 4u, 1000u})
            check_data(data, t, dataset_path);

    // Root and last node of SPO hashed, SOP (partial) hashed including its node 0.
    {
        auto c = build_index<index_c>(grid(5, 2, 6), 3);
        expect(c.tries[0].node_has_hash(0), "SPO root hashed");
        expect(c.tries[1].mphf_count() > 0 && c.tries[1].node_has_hash(0), "SOP (partial) hashed, node 0 included");
        check_pair(build_index<index_a>(grid(5, 2, 6), 3), c, "grid 5x2x6, t=3");
    }
    check_data(grid(5, 2, 6), 1000, "grid 5x2x6 (no MPHF)");
    check_data(grid(5, 1, 2), 3, "grid 5x1x2 (SOP without MPHFs, root permutation)");

    for (uint32_t seed = 1; seed <= 6; seed++)
        for (uint32_t t : {2u, 5u, 20u, 100u})
            check_data(random_triples(seed, 3000, 60), t, "random seed=" + std::to_string(seed) + " n=3000 range=60");

    // An index of one policy does not load with the other.
    {
        auto a = build_index<index_a>(grid(5, 2, 6), 3);
        auto c = build_index<index_c>(grid(5, 2, 6), 3);
        auto expect_rejected = [](auto& from, auto& into, const std::string& what) {
            std::stringstream ss;
            from.serialize(ss);
            try {
                into.load(ss);
                expect(false, what + " is rejected");
            } catch (const std::runtime_error& e) {
                std::cout << "  rejected (" << what << "): " << e.what() << "\n";
            }
        };
        index_c into_c;
        index_a into_a;
        expect_rejected(a, into_c, "A index loaded as C");
        expect_rejected(c, into_a, "C index loaded as A");
    }

    std::cout << (failures == 0 ? "ALL PASS" : "FAILURES: " + std::to_string(failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
