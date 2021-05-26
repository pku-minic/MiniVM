#include "mem/dense.h"

#include <cstdlib>
#include <cassert>

void DenseMemoryPool::FreeMems() {
  std::free(mems_);
}

MemId DenseMemoryPool::Allocate(std::uint32_t size) {
  // allocate memory
  auto id = mem_size_;
  mem_size_ += size;
  mems_ = reinterpret_cast<std::uint8_t *>(std::realloc(mems_, mem_size_));
  return id;
}

void *DenseMemoryPool::GetAddress(MemId id) {
  if (id >= mem_size_) return nullptr;
  return mems_ + id;
}

void DenseMemoryPool::SaveState() {
  states_.push(mem_size_);
}

void DenseMemoryPool::RestoreState() {
  auto last_size = states_.top();
  states_.pop();
  mems_ = reinterpret_cast<std::uint8_t *>(std::realloc(mems_, last_size));
}
