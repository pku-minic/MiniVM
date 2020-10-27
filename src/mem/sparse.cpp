#include "mem/sparse.h"

bool SparseMemoryPool::Allocate(SymId sym, std::uint32_t size) {
  if (!ids_.insert({sym, mems_.size()}).second) return false;
  mems_.push_back(std::make_unique<std::uint8_t[]>(size));
  return true;
}

std::optional<std::uint32_t> SparseMemoryPool::GetMemId(SymId sym) const {
  auto it = ids_.find(sym);
  if (it != ids_.end()) return it->second;
  return {};
}

void *SparseMemoryPool::GetAddressBySym(SymId sym) const {
  auto it = ids_.find(sym);
  if (it == ids_.end()) return nullptr;
  return mems_[it->second].get();
}

void *SparseMemoryPool::GetAddressById(std::uint32_t id) const {
  if (id >= mems_.size()) return nullptr;
  return mems_[id].get();
}
