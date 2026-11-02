// Binary stream of the LOUDS bitvector B and the labels L of every trie of an H-CLTJ index.
//
// Format, native endianness, one record per trie in index order (SPO, SOP, POS, PSO, OSP, OPS):
//   header      label_stream::header (32 bytes)
//   B           ceil(b_bits / 64) uint64_t words; bit i of B is bit (i % 64) of word i / 64,
//               bits past b_bits are 0. B is complete, trailing LOUDS sentinel included.
//   L           n_labels uint32_t, L[0 .. n_labels) in position order. The mock label that
//               m_seq carries after the last real label is not part of the stream.
// The stream ends right after the last record.
#ifndef CLTJ_UTIL_LABEL_STREAM_HPP
#define CLTJ_UTIL_LABEL_STREAM_HPP

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace cltj {
namespace label_stream {

static constexpr uint64_t MAGIC = 0x31534c4a544c4348ULL;  // "HCLTJLS1"
static constexpr uint32_t N_TRIES = 6;

struct header {
    uint64_t magic = MAGIC;
    uint32_t trie = 0;
    uint32_t reserved = 0;
    uint64_t b_bits = 0;
    uint64_t n_labels = 0;
};
static_assert(sizeof(header) == 32, "header layout is part of the format");

inline uint64_t b_words(uint64_t b_bits) { return (b_bits + 63) / 64; }

class writer {
  public:
    explicit writer(std::FILE* out) : out_(out) {}

    void write_header(const header& h) { write(&h, sizeof(h), 1); }
    void write_b_words(const uint64_t* words, size_t n) { write(words, sizeof(uint64_t), n); }
    void write_labels(const uint32_t* labels, size_t n) { write(labels, sizeof(uint32_t), n); }

    void flush() {
        if (std::fflush(out_) != 0)
            throw std::runtime_error(std::string("label_stream: flush failed: ") + std::strerror(errno));
    }

  private:
    std::FILE* out_;

    void write(const void* data, size_t size, size_t n) {
        if (n > 0 && std::fwrite(data, size, n, out_) != n)
            throw std::runtime_error(std::string("label_stream: write failed: ") + std::strerror(errno));
    }
};

// Sequential reader: per trie, next_trie(), then all of B, then all of L, in chunks of any size.
class reader {
  public:
    explicit reader(std::FILE* in) : in_(in) {}

    // False only at a clean end of stream (no byte left before a header).
    bool next_trie(header& h) {
        if (b_left_ != 0 || labels_left_ != 0)
            throw std::runtime_error("label_stream: previous trie not fully read");
        size_t got = std::fread(&h, 1, sizeof(h), in_);
        if (got == 0 && std::feof(in_))
            return false;
        if (got != sizeof(h))
            throw std::runtime_error("label_stream: truncated header");
        if (h.magic != MAGIC)
            throw std::runtime_error("label_stream: bad magic");
        if (h.trie != expected_trie_)
            throw std::runtime_error(
                "label_stream: expected trie " + std::to_string(expected_trie_) + ", got " +
                std::to_string(h.trie)
            );
        expected_trie_++;
        b_left_ = b_words(h.b_bits);
        labels_left_ = h.n_labels;
        return true;
    }

    void read_b_words(uint64_t* dst, size_t n) {
        if (n > b_left_)
            throw std::runtime_error("label_stream: reading past the end of B");
        read(dst, sizeof(uint64_t), n);
        b_left_ -= n;
    }

    void read_labels(uint32_t* dst, size_t n) {
        if (b_left_ != 0)
            throw std::runtime_error("label_stream: B not fully read before L");
        if (n > labels_left_)
            throw std::runtime_error("label_stream: reading past the end of L");
        read(dst, sizeof(uint32_t), n);
        labels_left_ -= n;
    }

    uint64_t b_words_left() const { return b_left_; }
    uint64_t labels_left() const { return labels_left_; }

  private:
    std::FILE* in_;
    uint32_t expected_trie_ = 0;
    uint64_t b_left_ = 0;
    uint64_t labels_left_ = 0;

    void read(void* dst, size_t size, size_t n) {
        if (n > 0 && std::fread(dst, size, n, in_) != n)
            throw std::runtime_error("label_stream: truncated stream");
    }
};

}  // namespace label_stream
}  // namespace cltj

#endif
