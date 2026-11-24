// Checks the per-node structures of bench/node_lookup.hpp against an oracle
// (binary search over the sorted labels): membership, position and successor,
// with present and absent labels, monotone seeks from a cursor and copies.

#include <bench/node_lookup.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace cltj::node_lookup;

namespace {

uint64_t failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++failures;
        if (failures < 20)
            std::cerr << "FAIL: " << what << std::endl;
    }
}

std::vector<uint32_t> make_set(const std::string& kind, uint64_t n, std::mt19937_64& rng) {
    std::set<uint32_t> s;
    if (kind == "sparse") {
        while (s.size() < n)
            s.insert(rng() % (1ULL << 31));
    } else if (kind == "dense") {
        while (s.size() < n)
            s.insert(1000 + rng() % (n + n / 2 + 1));
    } else if (kind == "runs") {
        uint32_t x = rng() % 1000;
        while (s.size() < n) {
            uint64_t len = 1 + rng() % 64;
            for (uint64_t k = 0; k < len && s.size() < n; ++k)
                s.insert(x++);
            x += 1 + rng() % 5000;
        }
    } else if (kind == "edges") {
        s.insert(0);
        s.insert(uint32_t(1) << 31);
        while (s.size() < n)
            s.insert(rng() % (1ULL << 31));
    }
    return std::vector<uint32_t>(s.begin(), s.end());
}

std::vector<uint32_t> probe_values(const std::vector<uint32_t>& keys, std::mt19937_64& rng) {
    std::vector<uint32_t> v;
    for (uint32_t k : keys) {
        v.push_back(k);
        if (k > 0)
            v.push_back(k - 1);
        v.push_back(k + 1);
    }
    for (int i = 0; i < 2000; ++i)
        v.push_back(keys.front() + rng() % (uint64_t(keys.back()) - keys.front() + 1));
    v.push_back(keys.back() + 1000);
    if (keys.front() > 0)
        v.push_back(0);
    return v;
}

template <class S>
void check_ordered(const S& s, const std::vector<uint32_t>& keys, std::mt19937_64& rng, const std::string& tag) {
    const uint64_t n = keys.size();
    for (uint32_t c : probe_values(keys, rng)) {
        Answer a = s.lookup(c);
        uint64_t want = std::lower_bound(keys.begin(), keys.end(), c) - keys.begin();
        bool present = want < n && keys[want] == c;
        check(a.found == present, tag + " lookup found " + std::to_string(c));
        check(a.pos == want, tag + " lookup pos " + std::to_string(c));
        if (want < n)
            check(a.value == keys[want], tag + " lookup value " + std::to_string(c));
    }
    for (int run = 0; run < 200; ++run) {
        uint64_t a = 0, cur = no_cursor;
        uint64_t c = keys.front() > 0 ? rng() % (uint64_t(keys.front()) + 1) : 0;
        uint64_t span = uint64_t(keys.back()) - keys.front() + 1;
        uint64_t step = 1 + rng() % std::max<uint64_t>(1, span / (1 + rng() % 64));
        for (uint64_t it = 0; it < 4 * n + 100; ++it) {
            Answer r = s.seek(uint32_t(c), a, cur);
            uint64_t want = std::lower_bound(keys.begin(), keys.end(), uint32_t(c)) - keys.begin();
            check(r.pos == want, tag + " seek pos " + std::to_string(c));
            if (want == n)
                break;
            check(r.value == keys[want] && r.found == (keys[want] == c), tag + " seek value " + std::to_string(c));
            a = r.pos;
            cur = r.cur;
            uint64_t kind = rng() % 4;
            if (kind == 0)
                ;  // same target again
            else if (kind == 1)
                c = keys[a] + 1;
            else if (kind == 2 && a + 1 < n)
                c = keys[std::min<uint64_t>(n - 1, a + 1 + rng() % 16)];
            else
                c += step;
            if (c > uint64_t(keys.back()) + 1)
                c = uint64_t(keys.back()) + 1;
            if (c > 0xffffffffULL)
                break;
        }
    }
}

template <class H>
void check_hash(const H& h, const std::vector<uint32_t>& keys, std::mt19937_64& rng, const std::string& tag) {
    std::vector<uint8_t> seen(keys.size(), 0);
    for (uint32_t k : keys) {
        Answer a = h.lookup(k);
        check(a.found && a.pos < keys.size(), tag + " present " + std::to_string(k));
        if (a.found && a.pos < keys.size()) {
            check(!seen[a.pos], tag + " slot repeated");
            seen[a.pos] = 1;
        }
    }
    for (uint32_t c : probe_values(keys, rng)) {
        bool present = std::binary_search(keys.begin(), keys.end(), c);
        check(h.lookup(c).found == present, tag + " membership " + std::to_string(c));
    }
}

template <class S>
S copy_of(const S& s) {
    S c = s.clone();
    std::vector<S> v;
    v.push_back(std::move(c));
    v.push_back(s.clone());  // forces a reallocation, which moves the first copy
    return std::move(v[0]);
}

}  // namespace

int main() {
    std::mt19937_64 rng(1234);
    const std::vector<std::string> kinds = {"sparse", "dense", "runs", "edges"};
    for (const auto& kind : kinds) {
        for (uint64_t n : {1ULL, 2ULL, 3ULL, 9ULL, 100ULL}) {
            if (kind == "edges" && n < 2)
                continue;
            auto keys = make_set(kind, n, rng);
            std::string tag = kind + "/" + std::to_string(n);
            check_ordered(EFNode(keys), keys, rng, "ef " + tag);
            check_ordered(GallopNode(keys, 32), keys, rng, "gallop " + tag);
        }
        for (uint64_t n : {4000ULL, 5003ULL, 65536ULL, 300000ULL}) {
            auto keys = make_set(kind, n, rng);
            std::string tag = kind + "/" + std::to_string(n);
            EFNode ef(keys);
            GallopNode gl(keys, 32);
            GlGhNode g(keys);
            PackedGlGhNode p(keys);
            check_ordered(ef, keys, rng, "ef " + tag);
            check_ordered(copy_of(ef), keys, rng, "ef copy " + tag);
            check_ordered(gl, keys, rng, "gallop " + tag);
            check_ordered(copy_of(gl), keys, rng, "gallop copy " + tag);
            check_hash(g, keys, rng, "glgh " + tag);
            check_hash(copy_of(g), keys, rng, "glgh copy " + tag);
            check_hash(p, keys, rng, "packed " + tag);
            check_hash(copy_of(p), keys, rng, "packed copy " + tag);
            std::cout << tag << ": ef " << 8.0 * ef.size_in_bytes() / n << " b/k (wl " << ef.low_width()
                      << "), glgh " << 8.0 * g.size_in_bytes() / n << ", packed " << 8.0 * p.size_in_bytes() / n
                      << ", residual " << g.mphf().n_residual() << std::endl;
        }
    }
    if (failures) {
        std::cout << "FAILURES: " << failures << std::endl;
        return 1;
    }
    std::cout << "all checks passed" << std::endl;
    return 0;
}
