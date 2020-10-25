#include "vm/symbol.h"

std::uint32_t SymbolPool::LogId(std::string_view symbol) {
  // try to find symbol
  auto it = defs_.find(symbol);
  if (it != defs_.end()) {
    return it->second;
  }
  else {
    // store to pool
    std::uint32_t id = pool_.size();
    pool_.emplace_back(symbol);
    // update symbol definition
    defs_[pool_.back()] = id;
  }
}

std::optional<std::uint32_t> SymbolPool::FindId(
    std::string_view symbol) const {
  auto it = defs_.find(symbol);
  if (it != defs_.end()) return it->second;
  return {};
}

std::optional<std::string_view> SymbolPool::FindSymbol(
    std::uint32_t id) const {
  if (id < pool_.size()) return pool_[id];
  return {};
}
