// Reads a label stream from stdin and checks it against the index it was dumped from.
// B is compared against compact_metatrie_hash::print(), which reads m_bv directly, so the
// zero walk of dump-labels-hcltj is checked against an independent path. Small indices only.
// Usage: dump-labels-hcltj <index> | test-label-stream <index>
#include <index/cltj_index_metatrie.hpp>
#include <util/label_stream.hpp>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string bits_from_print(cltj::compact_metatrie_hash* trie) {
    std::ostringstream captured;
    auto* old = std::cout.rdbuf(captured.rdbuf());
    trie->print();
    std::cout.rdbuf(old);
    std::string s = captured.str();
    if (!s.empty() && s.back() == '\n')
        s.pop_back();
    return s;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-index.hcltj>  (stream on stdin)" << std::endl;
        return 1;
    }

    cltj::compact_ltj_metatrie_hash index;
    sdsl::load_from_file(index, argv[1]);

    cltj::label_stream::reader r(stdin);
    uint64_t failures = 0;
    try {
        cltj::label_stream::header h;
        for (uint32_t t = 0; t < cltj::label_stream::N_TRIES; t++) {
            if (!r.next_trie(h)) {
                std::cerr << "FAIL: stream ended after " << t << " tries" << std::endl;
                return 1;
            }
            auto* trie = index.get_trie(t);
            const auto& seq = trie->seq;
            uint64_t trie_failures = 0;

            if (h.b_bits != trie->louds_size() || h.n_labels + 1 != seq.size()) {
                std::cerr << "  trie " << t << ": header b_bits=" << h.b_bits << " n_labels=" << h.n_labels
                          << ", index louds_size=" << trie->louds_size() << " seq.size=" << seq.size()
                          << std::endl;
                trie_failures++;
            }

            std::vector<uint64_t> words(cltj::label_stream::b_words(h.b_bits));
            r.read_b_words(words.data(), words.size());
            std::string expected_bits = bits_from_print(trie);
            if (expected_bits.size() != h.b_bits)
                trie_failures++;
            for (uint64_t i = 0; i < h.b_bits && i < expected_bits.size(); i++) {
                char got = ((words[i / 64] >> (i % 64)) & 1) ? '1' : '0';
                if (got != expected_bits[i])
                    trie_failures++;
            }
            if (h.b_bits % 64 != 0 && (words.back() >> (h.b_bits % 64)) != 0)
                trie_failures++;

            std::vector<uint32_t> labels(h.n_labels);
            r.read_labels(labels.data(), labels.size());
            for (uint64_t i = 0; i < h.n_labels; i++)
                if (labels[i] != seq[i])
                    trie_failures++;

            std::cout << "  trie " << t << ": b_bits=" << h.b_bits << " n_labels=" << h.n_labels
                      << (trie_failures == 0 ? "  PASS" : "  FAIL") << std::endl;
            failures += trie_failures;
        }
        if (r.next_trie(h)) {
            std::cerr << "FAIL: stream has more than " << cltj::label_stream::N_TRIES << " tries" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << std::endl;
        return 1;
    }

    std::cout << (failures == 0 ? "RESULT: ALL PASS" : "RESULT: FAILURES DETECTED") << std::endl;
    return failures == 0 ? 0 : 1;
}
