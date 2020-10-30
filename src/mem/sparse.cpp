#include "mem/sparse.h"

#include <cassert>

MemId SparseMemoryPool::Allocate(std::uint32_t size) {
  // allocate memory
  auto id = mem_size_;
  auto mem = std::make_unique<std::uint8_t[]>(size);
  auto ret = mems_.insert({id, std::move(mem)}).second;
  static_cast<void>(ret);
  assert(ret);
  // update allocated memory size
  mem_size_ += size;
  return id;
}

void *SparseMemoryPool::GetAddress(MemId id) {
  if (id >= mem_size_) return nullptr;
  auto it = mems_.lower_bound(id);
  if (it == mems_.end()) return nullptr;
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
  if (it == mems_.end()) return;
  mems_.erase(mems_.begin(), ++it);
}
