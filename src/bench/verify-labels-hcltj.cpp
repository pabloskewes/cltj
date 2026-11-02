// Checks an H-CLTJ index against the B and L of an index of the same dataset and threshold
// that still stored L, read from stdin as a label stream (util/label_stream.hpp). Per trie:
//   - B of the index equals B of the stream;
//   - in position order, L_plain gives back the block of every non-hashed node and the key
//     cursor the block of every hashed node, slot k against L[v + k] (overlay::check_labels);
//   - the blocks cover the n_labels of the stream and the hashed nodes are mphf_count().
// Usage: dump-labels-hcltj <old.hcltj> | verify-labels-hcltj <new.hcltj>
#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <metatrie/overlay/checks.hpp>
#include <util/label_stream.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* trie_names[6] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};
constexpr size_t CHUNK = size_t(1) << 20;

// Labels of the current trie of the stream, one at a time, read in chunks.
class chunked_labels {
  public:
    explicit chunked_labels(cltj::label_stream::reader& rd) : rd_(rd), buf_(CHUNK) {}

    uint32_t operator()() {
        if (pos_ == len_) {
            len_ = std::min<uint64_t>(buf_.size(), rd_.labels_left());
            if (len_ == 0)
                throw std::runtime_error("the blocks of the index go past the end of L in the stream");
            rd_.read_labels(buf_.data(), len_);
            pos_ = 0;
        }
        return buf_[pos_++];
    }

  private:
    cltj::label_stream::reader& rd_;
    std::vector<uint32_t> buf_;
    size_t pos_ = 0;
    size_t len_ = 0;
};

void drain_labels(cltj::label_stream::reader& rd) {
    std::vector<uint32_t> buf(CHUNK);
    while (rd.labels_left() > 0) {
        size_t n = std::min<uint64_t>(buf.size(), rd.labels_left());
        rd.read_labels(buf.data(), n);
    }
}

// Compares B word by word; bits past b_bits are ignored. Returns the number of differing words.
uint64_t compare_b(
    cltj::label_stream::reader& rd, const sdsl::bit_vector& bv, uint64_t b_bits, uint64_t max_messages,
    cltj::overlay::CheckReport& rep
) {
    std::vector<uint64_t> buf(CHUNK);
    const uint64_t* words = bv.data();
    const uint64_t n_words = cltj::label_stream::b_words(b_bits);
    uint64_t bad = 0;
    for (uint64_t w = 0; w < n_words;) {
        size_t n = std::min<uint64_t>(buf.size(), n_words - w);
        rd.read_b_words(buf.data(), n);
        for (size_t j = 0; j < n; j++, w++) {
            uint64_t mask = ~0ULL;
            if (w + 1 == n_words && b_bits % 64 != 0)
                mask = (1ULL << (b_bits % 64)) - 1;
            const uint64_t diff = (words[w] ^ buf[j]) & mask;
            rep.comparisons++;
            if (diff != 0) {
                bad++;
                if (bad <= max_messages) {
                    std::ostringstream m;
                    m << "B differs at bit " << w * 64 + __builtin_ctzll(diff);
                    rep.fail(m.str());
                } else {
                    rep.errors++;
                }
            }
        }
    }
    return bad;
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Check B, L_plain and the key cursors of an H-CLTJ index against a label stream on stdin"};
    std::string index_file;
    uint64_t details = 8;
    app.add_option("index", index_file, "H-CLTJ index file (.hcltj) built by this binary")->required();
    app.add_option("--details", details, "Mismatches printed in detail per trie")->capture_default_str();
    CLI11_PARSE(app, argc, argv);

    try {
        std::cerr << "Loading index: " << index_file << std::endl;
        cltj::compact_ltj_metatrie_hash index;
        if (!sdsl::load_from_file(index, index_file)) {
            std::cerr << "ERROR: cannot load " << index_file << std::endl;
            return 2;
        }
        std::cerr << "Index loaded: " << sdsl::size_in_bytes(index) << " bytes" << std::endl;

        std::setvbuf(stdin, nullptr, _IOFBF, size_t(1) << 24);
        cltj::label_stream::reader rd(stdin);
        bool all_ok = true;

        for (uint32_t t = 0; t < cltj::label_stream::N_TRIES; t++) {
            cltj::label_stream::header h;
            if (!rd.next_trie(h))
                throw std::runtime_error("the stream ends before trie " + std::to_string(t));
            const auto* trie = index.get_trie(t);

            cltj::overlay::CheckReport b_rep;
            b_rep.max_messages = details;
            uint64_t b_bad_words = 0;
            const bool same_b_size = h.b_bits == trie->louds_size();
            if (same_b_size) {
                b_bad_words = compare_b(rd, trie->bv(), h.b_bits, details, b_rep);
            } else {
                std::ostringstream m;
                m << "|B| " << trie->louds_size() << " vs " << h.b_bits << " in the stream";
                b_rep.fail(m.str());
                std::vector<uint64_t> buf(CHUNK);
                while (rd.b_words_left() > 0) {
                    size_t n = std::min<uint64_t>(buf.size(), rd.b_words_left());
                    rd.read_b_words(buf.data(), n);
                }
            }

            // With a different B the walk of the index does not line up with L: L is skipped.
            cltj::overlay::CheckReport rep;
            rep.max_messages = details;
            bool labels_checked = false;
            if (b_rep.ok() && h.n_labels + 1 == h.b_bits) {
                chunked_labels truth(rd);
                rep = cltj::overlay::check_labels(*trie, [&truth]() { return truth(); }, details);
                labels_checked = true;
            } else if (b_rep.ok()) {
                std::ostringstream m;
                m << "n_labels=" << h.n_labels << " in the stream, |B|-1=" << h.b_bits - 1;
                rep.fail(m.str());
            }
            const uint64_t left = rd.labels_left();
            drain_labels(rd);

            const uint64_t covered = rep.plain_labels + rep.hashed_labels;
            if (labels_checked) {
                rep.comparisons += 3;
                if (left != 0) {
                    std::ostringstream m;
                    m << left << " labels of the stream left after the last block";
                    rep.fail(m.str());
                }
                if (covered != h.n_labels) {
                    std::ostringstream m;
                    m << "blocks cover " << covered << " labels, n_labels=" << h.n_labels;
                    rep.fail(m.str());
                }
                if (rep.hashed_nodes != trie->mphf_count()) {
                    std::ostringstream m;
                    m << rep.hashed_nodes << " hashed nodes, mphf_count=" << trie->mphf_count();
                    rep.fail(m.str());
                }
            }

            const bool ok = b_rep.ok() && labels_checked && rep.ok();
            all_ok = all_ok && ok;
            std::cout << "--- Trie " << trie_names[t] << ": " << (ok ? "PASS" : "FAIL") << " ---\n"
                      << "  stream: b_bits=" << h.b_bits << " n_labels=" << h.n_labels << "\n"
                      << "  index:  louds_size=" << trie->louds_size() << " |L_plain|=" << trie->seq_plain().size()
                      << " mphfs=" << trie->mphf_count() << "\n"
                      << "  B: " << (b_rep.ok() ? "equal" : "DIFFERS") << " (" << b_bad_words << " words differ)\n";
            for (const auto& m : b_rep.messages)
                std::cout << "    " << m << "\n";
            std::cout << "  labels: " << (labels_checked ? "" : "not checked, ") << "covered=" << covered
                      << " plain_nodes=" << rep.plain_nodes << " plain_labels=" << rep.plain_labels
                      << " hashed_nodes=" << rep.hashed_nodes << " hashed_labels=" << rep.hashed_labels
                      << " comparisons=" << rep.comparisons << " mismatches=" << rep.errors << "\n";
            for (const auto& m : rep.messages)
                std::cout << "    " << m << "\n";
            std::cout << std::flush;
        }

        cltj::label_stream::header extra;
        if (rd.next_trie(extra))
            throw std::runtime_error("the stream has more than six tries");

        std::cout << (all_ok ? "RESULT: ALL PASS" : "RESULT: FAIL") << std::endl;
        return all_ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cout << "RESULT: ERROR" << std::endl;
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    }
}
