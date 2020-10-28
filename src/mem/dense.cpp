#include "mem/dense.h"

#include <cassert>

std::vector<std::uint8_t> DenseMemoryPool::mems_;

void DenseMemoryPool::Release() {
  // release all allocated memories in current memory pool
  mems_.resize(last_mem_size_);
}

bool DenseMemoryPool::Allocate(SymId sym, std::uint32_t size) {
  // make sure the symbol has not been allocated
  if (!ids_.insert({sym, mems_.size()}).second) return false;
  // allocate memory
  mems_.resize(mems_.size() + size);
  return true;
}

std::optional<MemId> DenseMemoryPool::GetMemId(SymId sym) const {
  auto it = ids_.find(sym);
  if (it != ids_.end()) return it->second;
  return {};
}

void *DenseMemoryPool::GetAddressBySym(SymId sym) const {
  // get memory id of the specific symbol
  auto it = ids_.find(sym);
  if (it == ids_.end()) return nullptr;
  // get address of memory
  assert(it->second < mems_.size());
  return mems_.data() + it->second;
}

void *DenseMemoryPool::GetAddressById(MemId id) const {
  if (id >= mems_.size()) return nullptr;
  return mems_.data() + id;
}
