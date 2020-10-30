#include "mem/dense.h"

#include <cassert>

MemId DenseMemoryPool::Allocate(std::uint32_t size) {
  // allocate memory
  auto id = mems_.size();
  mems_.resize(mems_.size() + size);
  return id;
}

void *DenseMemoryPool::GetAddress(MemId id) {
  if (id >= mems_.size()) return nullptr;
  return mems_.data() + id;
}

void DenseMemoryPool::SaveState() {
  states_.push(mems_.size());
}

void DenseMemoryPool::RestoreState() {
  mems_.resize(states_.top());
  states_.pop();
}
