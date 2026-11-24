#ifndef INTERSECTION_STATS_HPP
#define INTERSECTION_STATS_HPP

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>
#include <util/instrument.hpp>
#ifdef CLTJ_COLLECT_QUERY_STATS_ENABLED
#include <array>
#include <cltj_config.hpp>
#include <type_traits>
#endif

namespace ltj {

using cltj::COLLECT_QUERY_STATS;

/// @brief Code path that resolved a variable in the join.
enum class IntersectionPath : uint8_t {
  LEAPFROG,        ///< Leapfrog over every iterator.
  MIXED,           ///< Leapfrog over the sorted iterators, probe the hashed ones.
  PURE_HASH,       ///< Scan the smallest list, probe the other hashed ones.
  LONELY_SEEK_ALL, ///< Single iterator in its last level, enumerated by seek_all.
};

#ifdef CLTJ_COLLECT_QUERY_STATS_ENABLED
/// @brief What a frame did with one of its lists.
enum class ListRole : uint8_t {
  LEAPFROG, ///< Advanced with leap(), from a cursor.
  SORTED,   ///< Sorted list tested with exists() (MIXED_MIN).
  PROBE,    ///< Hashed list tested with hash_contains() and located with exists().
  ENUM,     ///< Hashed list enumerated with the slot cursor.
};

/// @brief Node whose children are a list: the trie that stores them, the
/// level of the labels (1 = children of the root), the parent's position in
/// that trie's LOUDS and the path labels in that trie's order (-1 where they
/// do not apply). leap() positions are relative to @p begin.
struct ListNode {
  uint8_t trie = 0;
  uint8_t level = 0;
  int64_t parent_pos = -1;
  int64_t label1 = -1;
  int64_t label2 = -1;
  uint64_t begin = 0;

  bool same_node(const ListNode &o) const {
    return trie == o.trie && level == o.level && label1 == o.label1 &&
           label2 == o.label2;
  }
};

/// @brief Bucket 0 holds 0; bucket b holds [2^(b-1), 2^b).
inline size_t log2_bucket(uint64_t x) {
  return x == 0 ? 0 : 64 - __builtin_clzll(x);
}
constexpr size_t LOG2_BUCKETS = 34;

/// @brief One list in one frame (a visit to its node).
struct ListVisit {
  const void *iter = nullptr;
  ListNode node;
  uint64_t n = 0;
  ListRole role = ListRole::LEAPFROG;
  /// Offset of the cursor in [0, n]; n once the list is exhausted.
  uint64_t cursor = 0;
  uint64_t seeks = 0;
  uint64_t moved = 0;
  uint64_t sum_jump = 0;
  std::array<uint32_t, LOG2_BUCKETS> jumps{};
  int first_jump_bucket = -1;
  uint64_t probes = 0;
  uint64_t probes_found = 0;
  uint64_t enumerated = 0;
  bool trace = false;
  /// With trace: (target, offset) per seek, or (key, found) per probe.
  std::vector<std::pair<uint32_t, uint32_t>> ops;

  void seek(uint64_t target, uint64_t offset) {
    const uint64_t jump = offset - cursor;
    const size_t b = log2_bucket(jump);
    if (seeks == 0)
      first_jump_bucket = b;
    ++seeks;
    moved += offset > cursor;
    sum_jump += jump;
    ++jumps[b];
    cursor = offset;
    if (trace)
      ops.emplace_back(target, offset);
  }

  void probe(uint64_t key, bool found) {
    ++probes;
    probes_found += found;
    if (trace)
      ops.emplace_back(key, found);
  }
};
#endif

/**
 * @brief Stores statistics for a single k-way set intersection operation (a
 * leapfrog join).
 *
 * This struct is designed to be memory-efficient by storing only essential
 * primary data. Derived statistics (like min, max, or average sizes) are
 * computed on-the-fly via member functions.
 */
struct IntersectionStats {

  // --- Contextual Data ---
  /// @brief The ID of the variable being eliminated in this step.
  uint8_t variable_id = 0;

  /// @brief Position in the Variable Elimination Order (VEO)
  int depth = 0;

  // --- Primary Data (Stored) ---

  /// @brief A vector containing the size of each of the k lists being
  /// intersected.
  std::vector<uint64_t> list_sizes;

  /// @brief The calculated alternation complexity for this intersection.
  int alternation_complexity = 0;

  /// @brief The number of elements in the resulting intersection set.
  uint64_t result_size = 0;

  /// @brief The number of "seeks" or iterations the leapfrog join required to
  /// complete.
  uint64_t leapfrog_seeks = 0;

  IntersectionPath path = IntersectionPath::LEAPFROG;

  /// @brief How many of the k iterators were on a node with an MPHF overlay.
  uint8_t hashed_iterators = 0;

  /// @brief Values tested against the hashed iterators: the leapfrog output
  /// over the sorted ones, or the whole smallest list on PURE_HASH.
  uint64_t candidates = 0;

#ifdef CLTJ_COLLECT_QUERY_STATS_ENABLED
  /// @brief One per iterator of the frame, in its order; empty when the
  /// iterator cannot name its node.
  std::vector<ListVisit> lists;

  ListVisit *find_list(const void *iter) {
    for (auto &l : lists)
      if (l.iter == iter)
        return &l;
    return nullptr;
  }
#endif

  // --- Derived Statistics (Computed on-the-fly) ---

  size_t k() const {
    return list_sizes.size();
  }

  uint64_t min_list_size() const {
    if (list_sizes.empty()) {
      return 0;
    }
    return *std::min_element(list_sizes.begin(), list_sizes.end());
  }

  uint64_t max_list_size() const {
    if (list_sizes.empty()) {
      return 0;
    }
    return *std::max_element(list_sizes.begin(), list_sizes.end());
  }

  double avg_list_size() const {
    if (list_sizes.empty()) {
      return 0.0;
    }
    const double sum =
        std::accumulate(list_sizes.begin(), list_sizes.end(), 0.0);
    return sum / k();
  }
};

#ifdef CLTJ_COLLECT_QUERY_STATS_ENABLED
template <class Iter, class = void>
struct names_list_nodes : std::false_type {};
template <class Iter>
struct names_list_nodes<Iter, std::void_t<decltype(&Iter::list_node)>>
    : std::true_type {};

/// @brief Fills stats.lists (needs stats.list_sizes) before the frame's first
/// leap; lists whose node is @p trace keep their operations.
template <class Iter, class Var>
void open_lists(IntersectionStats &stats, const std::vector<Iter *> &itrs,
                Var x, const ListNode *trace) {
  if constexpr (names_list_nodes<Iter>::value) {
    stats.lists.resize(itrs.size());
    for (size_t i = 0; i < itrs.size(); ++i) {
      Iter *it = itrs[i];
      const state_type st = it->is_variable_subject(x)     ? s
                            : it->is_variable_predicate(x) ? p
                                                           : o;
      ListVisit &l = stats.lists[i];
      l.iter = it;
      l.n = stats.list_sizes[i];
      l.node = it->list_node(st);
      l.trace = trace && trace->same_node(l.node);
    }
  }
}

/// @brief Records leap(x, target) on @p it, which returned @p value.
template <class Iter>
void track_leap(IntersectionStats *frame, Iter *it, uint64_t target,
                uint64_t value) {
  if constexpr (names_list_nodes<Iter>::value) {
    if (ListVisit *l = frame ? frame->find_list(it) : nullptr)
      l->seek(target, value == 0 ? l->n : it->leap_position() - l->node.begin);
  }
}

inline void close_lists(IntersectionStats &stats) {
  for (const auto &l : stats.lists)
    stats.leapfrog_seeks += l.seeks;
}
#endif

} // namespace ltj

#endif // INTERSECTION_STATS_HPP