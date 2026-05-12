#pragma once

#include <cstdint>

namespace util {

/**
 * @brief Count-only result collector for query benchmarks.
 *
 * This class is used to collect the number of results returned by a query.
 * It is used to measure the performance of the query engine without the overhead
 * of collecting and materializing the results.
 */
class results_counter {
  public:
    using size_type = uint64_t;

    results_counter() = default;

    template <class T>
    inline void add(const T&) {
        ++m_cnt;
    }

    inline size_type size() const { return m_cnt; }
    inline void clear() { m_cnt = 0; }

  private:
    size_type m_cnt = 0;
};

}  // namespace util
