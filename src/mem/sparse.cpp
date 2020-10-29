#include "mem/sparse.h"

#include <cassert>

MemId SparseMemoryPool::Allocate(std::uint32_t size) {
  // allocate memory
  auto mem = std::make_unique<std::uint8_t[]>(size);
  auto ret = mems_.insert({mem_size_, std::move(mem)}).second;
  static_cast<void>(ret);
  assert(ret);
  // update allocated memory size
  mem_size_ += size;
  return true;
}

void *SparseMemoryPool::GetAddress(MemId id) const {
  if (id >= mem_size_) return nullptr;
  auto it = mems_.upper_bound(id);
  if (it == mems_.begin()) return nullptr;
  --it;
  return it->second.get() + (id - it->first);
}

void SparseMemoryPool::SaveState() {
  states_.push(mem_size_);
}

void SparseMemoryPool::RestoreState() {
  // restore to the previous memory size
  mem_size_ = states_.top();
  states_.pop();
  // remove all allocated memory after current state
  auto it = mems_.find(mem_size_);
  assert(it != mems_.end());
  mems_.erase(it, mems_.end());
}
