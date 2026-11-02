// Writes B and L of the six tries of an H-CLTJ index to stdout, in the format of
// util/label_stream.hpp. Everything else goes to stderr.
#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <util/label_stream.hpp>

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

static constexpr size_t BLOCK_WORDS = 1 << 16;
static constexpr size_t BLOCK_LABELS = 1 << 17;

// m_bv is private, so B is rebuilt from its zeros: the next zero after z is z + children(z).
static void write_b(cltj::compact_metatrie_hash* trie, cltj::label_stream::writer& w) {
    const uint64_t b_bits = trie->louds_size();
    const uint64_t n_words = cltj::label_stream::b_words(b_bits);
    std::vector<uint64_t> block(BLOCK_WORDS);

    uint64_t zero = trie->child(0, 0);  // select0(1), the first zero
    for (uint64_t first = 0; first < n_words; first += BLOCK_WORDS) {
        const uint64_t n = std::min<uint64_t>(BLOCK_WORDS, n_words - first);
        const uint64_t end_bit = std::min(b_bits, (first + n) * 64);
        std::fill(block.begin(), block.begin() + n, ~0ULL);
        if (end_bit % 64 != 0)
            block[n - 1] = (1ULL << (end_bit % 64)) - 1;
        while (zero < end_bit) {
            block[zero / 64 - first] &= ~(1ULL << (zero % 64));
            zero += trie->children(zero);
        }
        w.write_b_words(block.data(), n);
    }
    if (zero != b_bits)
        throw std::runtime_error("zero walk over B did not end at louds_size()");
}

static void write_l(cltj::compact_metatrie_hash* trie, uint64_t n_labels, cltj::label_stream::writer& w) {
    const auto& seq = trie->seq;
    std::vector<uint32_t> block(BLOCK_LABELS);
    for (uint64_t first = 0; first < n_labels; first += BLOCK_LABELS) {
        const uint64_t n = std::min<uint64_t>(BLOCK_LABELS, n_labels - first);
        for (uint64_t i = 0; i < n; i++)
            block[i] = static_cast<uint32_t>(seq[first + i]);
        w.write_labels(block.data(), n);
    }
}

int main(int argc, char** argv) {
    CLI::App app{"Dump B and L of every trie of an H-CLTJ index to stdout (util/label_stream.hpp)"};
    std::string index_path;
    app.add_option("index", index_path, "Path to the .hcltj index")->required()->check(CLI::ExistingFile);
    CLI11_PARSE(app, argc, argv);

    // A closed pipe must surface as a failed write, not as a silent SIGPIPE death.
    std::signal(SIGPIPE, SIG_IGN);

    // The stream keeps the real stdout; anything else printed to fd 1 lands on stderr.
    int stream_fd = dup(STDOUT_FILENO);
    if (stream_fd < 0 || dup2(STDERR_FILENO, STDOUT_FILENO) < 0) {
        std::perror("dup");
        return 1;
    }
    std::FILE* out = fdopen(stream_fd, "wb");
    if (out == nullptr) {
        std::perror("fdopen");
        return 1;
    }

    try {
        std::cerr << "Loading index: " << index_path << std::endl;
        cltj::compact_ltj_metatrie_hash index;
        sdsl::load_from_file(index, index_path);

        cltj::label_stream::writer w(out);
        for (uint32_t t = 0; t < cltj::label_stream::N_TRIES; t++) {
            auto* trie = index.get_trie(t);
            const auto& seq = trie->seq;
            if (seq.size() == 0 || seq[seq.size() - 1] != 0)
                throw std::runtime_error("trie " + std::to_string(t) + ": m_seq does not end with the 0 mock");

            cltj::label_stream::header h;
            h.trie = t;
            h.b_bits = trie->louds_size();
            h.n_labels = seq.size() - 1;
            w.write_header(h);
            write_b(trie, w);
            write_l(trie, h.n_labels, w);
            std::cerr << "trie " << t << ": b_bits=" << h.b_bits << " n_labels=" << h.n_labels << std::endl;
        }
        w.flush();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    if (std::fclose(out) != 0) {
        std::perror("ERROR: close");
        return 1;
    }
    return 0;
}
