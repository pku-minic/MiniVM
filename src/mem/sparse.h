#ifndef MINIVM_MEM_SPARSE_H_
#define MINIVM_MEM_SPARSE_H_

#include <memory>
#include <vector>
#include <cstdint>

#include "mem/pool.h"

class SparseMemoryPool : public MemoryPoolInterface {
 public:
  SparseMemoryPool() {}

  std::uint32_t Allocate(std::uint32_t size) override;
  void *GetAddress(std::uint32_t id) const override;

 private:
  using BytesPtr = std::unique_ptr<std::uint8_t[]>;

  // all allocated memories
  std::vector<BytesPtr> mems_;
};

#endif  // MINIVM_MEM_SPARSE_H_
