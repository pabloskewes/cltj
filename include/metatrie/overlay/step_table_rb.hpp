#pragma once
#include "step_table.hpp"
#include "strategy.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace cltj {
namespace overlay {

/**
 * @brief Scheme C for tries whose root has no labels in L_plain (they live in a root_bitmap).
 *
 * Same table as StepTableOverlay, plus root_deleted: deg(root) in a full trie, whose root
 * block [0, deg(root)) is left out of L_plain, and 0 in a partial trie, whose node 0 is a
 * first-level node. Every node v > 0 then has
 *   deleted_before(v) = root_deleted + cum_deleted[r - 1].
 * The root is not hashed, so r keeps its meaning and the root has r = 0. plain_start(0) is
 * undefined when root_deleted > 0: the root has no block in L_plain.
 */
class StepTableRbOverlay : public OverlayPolicy<StepTableRbOverlay> {
  public:
    typedef uint64_t size_type;

    static constexpr uint8_t format_version = 5;
    static constexpr bool drops_root_labels = true;

  private:
    StepTableOverlay m_steps;
    uint64_t m_root_deleted = 0;

  public:
    void swap(StepTableRbOverlay& o) {
        m_steps.swap(o.m_steps);
        std::swap(m_root_deleted, o.m_root_deleted);
    }

    bool has_dict(size_type v) const { return m_steps.has_dict(v); }

    bool has_dict(size_type v, size_type r) const { return m_steps.has_dict(v, r); }

    size_type dict_idx(size_type v) const { return m_steps.dict_idx(v); }

    size_type plain_start(size_type v) const { return plain_start(v, dict_idx(v)); }

    size_type plain_start(size_type v, size_type r) const { return m_steps.plain_start(v, r) - m_root_deleted; }

    void begin(size_type louds_len, size_type plain_len) {
        m_steps.begin(louds_len, plain_len);
        m_root_deleted = 0;
    }

    //! In place of on_plain_node() for the root of a full trie, whose block leaves L_plain.
    void on_dropped_root(size_type d) { m_root_deleted = d; }

    void on_hashed_node(size_type new_pos, size_type d) { m_steps.on_hashed_node(new_pos, d); }

    void on_plain_node(size_type, size_type) {}

    void finalize(const sdsl::bit_vector&, uint32_t) {}

    void bind(const sdsl::bit_vector&) {}

    size_type hashed_count() const { return m_steps.hashed_count(); }

    size_type hashed_start(size_type r) const { return m_steps.hashed_start(r); }

    size_type cum_deleted(size_type r) const { return m_steps.cum_deleted(r); }

    size_type root_deleted() const { return m_root_deleted; }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, const std::string& name = "")
        const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = m_steps.serialize(out, child, "steps");
        written_bytes += sdsl::write_member(m_root_deleted, out, child, "root_deleted");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        m_steps.load(in);
        sdsl::read_member(m_root_deleted, in);
        if (!in)
            throw std::runtime_error("StepTableRbOverlay::load: truncated stream");
    }

    OverlaySizeBreakdown get_size_breakdown() const {
        OverlaySizeBreakdown b = m_steps.get_size_breakdown();
        b.components.push_back({"root_deleted", sizeof(m_root_deleted)});
        return b;
    }
};

}  // namespace overlay
}  // namespace cltj
