#include "mem/sparse.h"

#include <cassert>

std::uint32_t SparseMemoryPool::mem_size_ = 0;

bool SparseMemoryPool::Allocate(SymId sym, std::uint32_t size) {
  // make sure the symbol has not been allocated
  if (!ids_.insert({sym, mem_size_}).second) return false;
  // allocate memory
  auto mem = std::make_unique<std::uint8_t[]>(size);
  auto ret = mems_.insert({mem_size_, std::move(mem)}).second;
  static_cast<void>(ret);
  assert(ret);
  // update allocated memory size
  mem_size_ += size;
  return true;
}

std::optional<MemId> SparseMemoryPool::GetMemId(SymId sym) const {
  auto it = ids_.find(sym);
  if (it != ids_.end()) return it->second;
  return {};
}

void *SparseMemoryPool::GetAddressBySym(SymId sym) const {
  // get memory id of the specific symbol
  auto id_it = ids_.find(sym);
  if (id_it == ids_.end()) return nullptr;
  // get address of memory
  auto mem_it = mems_.find(id_it->second);
  assert(mem_it != mems_.end());
  return mem_it->second.get();
}

void *SparseMemoryPool::GetAddressById(MemId id) const {
  auto it = mems_.upper_bound(id);
  if (it == mems_.end()) return nullptr;
  return (--it)->second.get();
}
