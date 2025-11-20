#pragma once
#include "mphf_types.hpp"
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <iostream>
#include <vector>
#include <string>

namespace cltj {
namespace hashing {

template <typename Derived>
class GStorage {
  protected:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

  public:
    uint32_t get(uint32_t vertex) const { return derived().get(vertex); }
    void set(uint32_t vertex, uint32_t value) { derived().set(vertex, value); }
    void build(const std::vector<Triple>& peeling_order, uint32_t m) { derived().build(peeling_order, m); }
    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        return derived().serialize(out, v, name);
    }
    void load(std::istream& in) { derived().load(in); }
    size_t size_in_bytes() const { return derived().size_in_bytes(); }
};

}  // namespace hashing
}  // namespace cltj
