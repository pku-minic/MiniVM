#ifndef MINIVM_MEM_SPARSE_H_
#define MINIVM_MEM_SPARSE_H_

#include <memory>
#include <functional>
#include <map>
#include <stack>
#include <cstdint>

#include "mem/pool.h"

namespace minivm::mem {

// sparse memory pool
// no boundary check for any accessing operation
class SparseMemoryPool : public MemoryPoolInterface {
 public:
  SparseMemoryPool() : mem_size_(0) {}

  MemId Allocate(std::uint32_t size, bool init) override;
  void *GetAddress(MemId id) override;
  void SaveState() override;
  void RestoreState() override;

 private:
  using BytesPtr = std::unique_ptr<std::uint8_t[]>;

  // all allocated memory
  std::map<MemId, BytesPtr, std::greater<MemId>> mems_;
  // size of all allocated memory
  std::uint32_t mem_size_;
  // stack of saved states
  std::stack<std::uint32_t> states_;
};

}  // namespace minivm::mem

#endif  // MINIVM_MEM_SPARSE_H_
