#include "vm/symbol.h"

#include <utility>

void SymbolPool::PushNewSymbol(std::string_view symbol) {
  auto sym_ptr = std::make_unique<char[]>(symbol.size() + 1);
  symbol.copy(sym_ptr.get(), symbol.size());
  sym_ptr[symbol.size()] = '\0';
  pool_.push_back(std::move(sym_ptr));
}

void SymbolPool::Reset() {
  defs_.clear();
  pool_.clear();
}

SymId SymbolPool::LogId(std::string_view symbol) {
  // try to find symbol
  auto it = defs_.find(symbol);
  if (it != defs_.end()) {
    return it->second;
  }
  else {
    // store to pool
    SymId id = pool_.size();
    PushNewSymbol(symbol);
    // update symbol definition
    return defs_[pool_.back().get()] = id;
  }
}

std::optional<SymId> SymbolPool::FindId(std::string_view symbol) const {
  auto it = defs_.find(symbol);
  if (it != defs_.end()) return it->second;
  return {};
}

std::optional<std::string_view> SymbolPool::FindSymbol(SymId id) const {
  if (id < pool_.size()) return pool_[id].get();
  return {};
}
