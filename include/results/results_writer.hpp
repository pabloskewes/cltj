//
// Created by Adrián on 24/10/23.
//

#ifndef CLTJ_RESULTS_WRITER_HPP
#define CLTJ_RESULTS_WRITER_HPP

#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace util {

template <class Cons> class results_writer {

public:
  typedef uint64_t size_type;

private:
  std::ofstream m_stream;
  size_type m_cnt = 0;

  void copy(const results_writer &o) {
    m_stream = o.m_stream;
    m_cnt = o.m_cnt;
  }

public:
  results_writer(std::string &file) {
    m_stream.open(file, std::ios::binary);
  }

  inline void add(const std::vector<Cons> &val) {
    m_stream.write((char *)val.data(), sizeof(Cons) * val.size());
    ++m_cnt;
  };

  template <class Var>
  inline void add(const std::vector<std::pair<Var, Cons>> &val) {
    std::vector<Cons> values(val.size());
    for (const auto &pair : val) {
      values[pair.first] = pair.second;
    }
    m_stream.write((char *)values.data(), sizeof(Cons) * values.size());
    ++m_cnt;
  };

  inline size_type size() {
    return m_cnt;
  }

  inline void close() {
    m_stream.close();
  }

  //! Copy constructor
  results_writer(const results_writer &o) {
    copy(o);
  }

  //! Move constructor
  results_writer(results_writer &&o) {
    *this = std::move(o);
  }

  //! Copy Operator=
  results_writer &operator=(const results_writer &o) {
    if (this != &o) {
      copy(o);
    }
    return *this;
  }

  //! Move Operator=
  results_writer &operator=(results_writer &&o) {
    if (this != &o) {
      m_stream = std::move(o.m_stream);
      m_cnt = std::move(o.m_cnt);
    }
    return *this;
  }

  void swap(results_writer &o) {
    std::swap(m_stream, o.m_stream);
    std::swap(m_cnt, o.m_cnt);
  }
};

} // namespace util
#endif // CLTJ_RESULTS_WRITER_HPP