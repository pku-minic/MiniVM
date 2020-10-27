#include "mem/sparse.h"

std::uint32_t SparseMemoryPool::Allocate(std::uint32_t size) {
  mems_.push_back(std::make_unique<std::uint8_t[]>(size));
  return mems_.size() - 1;
}

void *SparseMemoryPool::GetAddress(std::uint32_t id) const {
  if (id >= mems_.size()) return nullptr;
  return mems_[id].get();
}
