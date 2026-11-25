#ifndef LTJ_INTERSECTION_CACHE_HPP
#define LTJ_INTERSECTION_CACHE_HPP

// Cache of the last intersection (k = 1) of the leapfrog frames, keyed by S: the node of each iterator of the
// frame, in the order of the iterators. Two frames with the same S yield the same results, in the same order,
// with the iterators at the same positions.
//   LTJ_ICACHE_VAR   one entry per variable, used when the variable of the previous depth is not in its patterns
//   LTJ_ICACHE_NODE  a table of slots indexed by node; a frame hits if the slot of one of its nodes holds its S
//   LTJ_ICACHE_CHECK hashes the state of the iterators before each down() of a frame, to compare runs

#if defined(LTJ_ICACHE_VAR) && defined(LTJ_ICACHE_NODE)
#error "LTJ_ICACHE_VAR and LTJ_ICACHE_NODE are exclusive"
#endif
#if defined(LTJ_ICACHE_VAR) || defined(LTJ_ICACHE_NODE)
#define LTJ_ICACHE 1
#endif
#if defined(LTJ_ICACHE) || defined(LTJ_ICACHE_CHECK)
#define LTJ_ICACHE_HOOKS 1
#endif
#if defined(LTJ_ICACHE_STATS) && !defined(LTJ_ICACHE)
#error "LTJ_ICACHE_STATS needs LTJ_ICACHE_VAR or LTJ_ICACHE_NODE"
#endif

#ifndef LTJ_ICACHE_CAP
#define LTJ_ICACHE_CAP 65536
#endif
#ifndef LTJ_ICACHE_NODE_SLOTS
#define LTJ_ICACHE_NODE_SLOTS (1ULL << 20)
#endif
#ifndef LTJ_ICACHE_NODE_BUDGET
#define LTJ_ICACHE_NODE_BUDGET (1ULL << 30)
#endif

#ifdef LTJ_ICACHE_STATS
#define LTJ_ICACHE_OP(n) (*m_ic_ops += (n))
#else
#define LTJ_ICACHE_OP(n) ((void)0)
#endif

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <memory>
#include <new>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace ltj::icache {

// Distinct for distinct nodes of one index: the position of the parent in the LOUDS of the trie that holds the
// list, that trie, and the level of the list (1 for the root, whose parent position is 0).
inline uint64_t node_key(uint64_t parent_pos, uint64_t trie, uint64_t level) {
    assert(parent_pos < (1ULL << 59) && trie < 8 && level < 4);
    return parent_pos << 5 | trie << 2 | level;
}

inline void mix(uint64_t& h, uint64_t x) {
    for (int i = 0; i < 8; ++i) {
        h ^= (x >> (8 * i)) & 0xff;
        h *= 1099511628211ULL;
    }
}

struct stats_type {
    uint64_t frames = 0;
    uint64_t lookups = 0;
    uint64_t hits = 0;
    uint64_t miss_empty = 0;
    uint64_t miss_other_node = 0;
    uint64_t miss_other_s = 0;
    uint64_t published = 0;
    uint64_t drop_cap = 0;
    uint64_t drop_budget = 0;
    uint64_t served = 0;
    uint64_t hit_lists = 0;
    uint64_t ops_saved = 0;
    uint64_t ops_done = 0;
    uint64_t peak_bytes = 0;

    static void header(std::ostream& os) {
        os << "ICACHE;query;frames;lookups;hits;miss_empty;miss_other_node;miss_other_s;published;drop_cap;"
              "drop_budget;served;hit_lists;ops_saved;ops_done;peak_bytes\n";
    }
    void print(std::ostream& os, uint64_t query) const {
        os << "ICACHE;" << query << ";" << frames << ";" << lookups << ";" << hits << ";" << miss_empty << ";"
           << miss_other_node << ";" << miss_other_s << ";" << published << ";" << drop_cap << ";" << drop_budget
           << ";" << served << ";" << hit_lists << ";" << ops_saved << ";" << ops_done << ";" << peak_bytes << "\n";
    }
};

struct check_type {
    uint64_t results = 0;
    uint64_t result_hash = 1469598103934665603ULL;
    uint64_t positioned = 0;
    uint64_t state_hash = 1469598103934665603ULL;

    void print(std::ostream& os, uint64_t query) const {
        os << "CHECK;" << query << ";" << results << ";" << result_hash << ";" << positioned << ";" << state_hash
           << "\n";
    }
};

// Layout of m_data: S (k nodes), then (end, cnt) of the next level of each iterator, then one record per result:
// the value and the position (beg) of each iterator in the next level.
class entry {
    std::vector<uint64_t> m_data;
    uint32_t m_k = 0;

  public:
    uint32_t refs = 0;
    bool valid = false;
    bool full = false;
    uint64_t ops = 0;
    uint64_t accounted = 0;

    void start(const uint64_t* nodes, uint32_t k) {
        m_k = k;
        m_data.assign(nodes, nodes + k);
        m_data.resize(3 * k);
        full = false;
        ops = 0;
    }
    bool same(const uint64_t* nodes, uint32_t k) const {
        return m_k == k && std::equal(nodes, nodes + k, m_data.begin());
    }
    uint32_t k() const { return m_k; }
    const uint64_t* nodes() const { return m_data.data(); }
    uint64_t* level() { return m_data.data() + m_k; }
    const uint64_t* level() const { return m_data.data() + m_k; }
    uint64_t size() const { return (m_data.size() - 3 * m_k) / (1 + m_k); }
    const uint64_t* result(uint64_t r) const { return m_data.data() + 3 * m_k + r * (1 + m_k); }
    uint64_t bytes() const { return m_data.capacity() * sizeof(uint64_t); }
    void release_memory() { std::vector<uint64_t>().swap(m_data); }

    // Room for one more record, or nullptr once the entry is past the cap (it stops recording).
    uint64_t* push() {
        if (full)
            return nullptr;
        if (size() == LTJ_ICACHE_CAP) {
            full = true;
            return nullptr;
        }
        m_data.resize(m_data.size() + 1 + m_k);
        return m_data.data() + m_data.size() - 1 - m_k;
    }
};

class var_cache {
    std::vector<entry> m_entry;
    std::vector<uint64_t> m_vars;  // bit v set iff variable v is in a pattern of the variable

  public:
    stats_type stats;

    template <class iterators_type>
    void add_variable(uint64_t x, const iterators_type& itrs) {
        uint64_t vars = 0;
        for (const auto* it : itrs) {
            const auto* t = it->get_triple_pattern();
            for (const auto* term : {&t->term_s, &t->term_p, &t->term_o}) {
                if (!term->is_variable)
                    continue;
                if (term->value >= 64)
                    throw std::runtime_error("LTJ_ICACHE_VAR supports variable ids below 64");
                vars |= 1ULL << term->value;
            }
        }
        if (m_vars.size() <= x) {
            m_vars.resize(x + 1, 0);
            m_entry.resize(x + 1);
        }
        m_vars[x] = vars;
    }

    bool eligible(uint64_t x, int64_t parent) const { return parent >= 0 && !(m_vars[x] >> parent & 1); }

    // The entry to replay, or nullptr with *fill set to the entry to record into.
    const entry* open(uint64_t x, const uint64_t* nodes, uint32_t k, entry** fill) {
        entry& e = m_entry[x];
#ifdef LTJ_ICACHE_STATS
        ++stats.lookups;
#endif
        if (e.valid && e.same(nodes, k)) {
#ifdef LTJ_ICACHE_STATS
            ++stats.hits;
#endif
            return &e;
        }
#ifdef LTJ_ICACHE_STATS
        ++(e.valid ? stats.miss_other_s : stats.miss_empty);
#endif
        e.valid = false;
        e.start(nodes, k);
        *fill = &e;
        return nullptr;
    }

    void publish(entry* e) {
        if (e->full) {
#ifdef LTJ_ICACHE_STATS
            ++stats.drop_cap;
#endif
            return;
        }
        e->valid = true;
#ifdef LTJ_ICACHE_STATS
        ++stats.published;
        uint64_t bytes = 0;
        for (const entry& v : m_entry)
            bytes += v.bytes();
        stats.peak_bytes = std::max(stats.peak_bytes, bytes);
#endif
    }

    void release(const entry*) {}
};

class node_cache {
    struct slot_type {
        uint64_t node;
        entry* e;
    };
    static_assert((LTJ_ICACHE_NODE_SLOTS & (LTJ_ICACHE_NODE_SLOTS - 1)) == 0, "LTJ_ICACHE_NODE_SLOTS is a power of 2");
    static constexpr int slot_bits = __builtin_ctzll(LTJ_ICACHE_NODE_SLOTS);

    struct free_deleter {
        void operator()(slot_type* p) const { std::free(p); }
    };
    // calloc: the pages of the table are zeroed lazily, so a query pays only for the slots it touches.
    std::unique_ptr<slot_type, free_deleter> m_table{
        static_cast<slot_type*>(std::calloc(LTJ_ICACHE_NODE_SLOTS, sizeof(slot_type)))
    };
    std::deque<entry> m_pool;
    std::vector<entry*> m_free;
    // Capacity of every entry of the pool, free ones included, so it bounds the memory the cache holds.
    uint64_t m_bytes = 0;

    slot_type& slot(uint64_t node) {
        return m_table.get()[(node * 0x9E3779B97F4A7C15ULL) >> (64 - slot_bits)];
    }

    void account(entry* e) {
        m_bytes += e->bytes() - e->accounted;
        e->accounted = e->bytes();
    }

    void unpin(entry* e) {
        if (--e->refs)
            return;
        account(e);
        if (m_bytes > LTJ_ICACHE_NODE_BUDGET) {
            m_bytes -= e->accounted;
            e->release_memory();
            e->accounted = 0;
        }
        m_free.push_back(e);
    }

  public:
    stats_type stats;

    node_cache() {
        if (!m_table)
            throw std::bad_alloc();
    }

    template <class iterators_type>
    void add_variable(uint64_t, const iterators_type&) {}

    bool eligible(uint64_t, int64_t) const { return true; }

    // The entry to replay (pinned until release()), or nullptr with *fill set to a pinned entry to record into.
    const entry* open(uint64_t, const uint64_t* nodes, uint32_t k, entry** fill) {
#ifdef LTJ_ICACHE_STATS
        ++stats.lookups;
        int reason = 0;  // 0 empty, 1 other node, 2 other S
#endif
        for (uint32_t i = 0; i < k; ++i) {
            const slot_type& s = slot(nodes[i]);
            if (!s.e)
                continue;
            if (s.node != nodes[i]) {
#ifdef LTJ_ICACHE_STATS
                reason = std::max(reason, 1);
#endif
                continue;
            }
            if (s.e->same(nodes, k)) {
                ++s.e->refs;
#ifdef LTJ_ICACHE_STATS
                ++stats.hits;
#endif
                return s.e;
            }
#ifdef LTJ_ICACHE_STATS
            reason = 2;
#endif
        }
#ifdef LTJ_ICACHE_STATS
        ++(reason == 0 ? stats.miss_empty : reason == 1 ? stats.miss_other_node : stats.miss_other_s);
#endif
        entry* e;
        if (m_free.empty()) {
            e = &m_pool.emplace_back();
        } else {
            e = m_free.back();
            m_free.pop_back();
        }
        e->start(nodes, k);
        e->refs = 1;
        *fill = e;
        return nullptr;
    }

    void publish(entry* e) {
        account(e);
        if (e->full) {
#ifdef LTJ_ICACHE_STATS
            ++stats.drop_cap;
#endif
        } else if (m_bytes > LTJ_ICACHE_NODE_BUDGET) {
#ifdef LTJ_ICACHE_STATS
            ++stats.drop_budget;
#endif
        } else {
            const uint64_t* nodes = e->nodes();
            for (uint32_t i = 0; i < e->k(); ++i) {
                slot_type& s = slot(nodes[i]);
                if (s.e != e) {
                    if (s.e)
                        unpin(s.e);
                    s.e = e;
                    ++e->refs;
                }
                s.node = nodes[i];
            }
#ifdef LTJ_ICACHE_STATS
            ++stats.published;
            stats.peak_bytes = std::max(stats.peak_bytes, m_bytes);
#endif
        }
        unpin(e);
    }

    void release(const entry* e) { unpin(const_cast<entry*>(e)); }
};

#if defined(LTJ_ICACHE_VAR)
typedef var_cache cache_type;
#elif defined(LTJ_ICACHE_NODE)
typedef node_cache cache_type;
#endif

}  // namespace ltj::icache

#endif  // LTJ_INTERSECTION_CACHE_HPP
