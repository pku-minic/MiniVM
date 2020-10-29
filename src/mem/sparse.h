#ifndef MINIVM_MEM_SPARSE_H_
#define MINIVM_MEM_SPARSE_H_

#include <memory>
#include <map>
#include <stack>
#include <cstdint>

#include "mem/pool.h"

// sparse memory pool
// no boundary check for any accessing operation
class SparseMemoryPool : public MemoryPoolInterface {
 public:
  SparseMemoryPool() : mem_size_(0) {}

  MemId Allocate(std::uint32_t size) override;
  void *GetAddress(MemId id) override;
  void SaveState() override;
  void RestoreState() override;

 private:
  using BytesPtr = std::unique_ptr<std::uint8_t[]>;

  // all allocated memory
  std::map<MemId, BytesPtr> mems_;
  // size of all allocated memory
  std::uint32_t mem_size_;
  // stack of saved states
  std::stack<std::uint32_t> states_;
};

#endif  // MINIVM_MEM_SPARSE_H_
