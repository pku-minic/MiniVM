#ifndef MINIVM_MEM_DENSE_H_
#define MINIVM_MEM_DENSE_H_

#include <vector>
#include <stack>
#include <cstdint>

#include "mem/pool.h"

// dense memory pool
// all memory will be allocated contiguously in one place
class DenseMemoryPool : public MemoryPoolInterface {
 public:
  DenseMemoryPool() {}

  MemId Allocate(std::uint32_t size) override;
  void *GetAddress(MemId id) override;
  void SaveState() override;
  void RestoreState() override;

 private:
  // all allocated memories
  std::vector<std::uint8_t> mems_;
  // stack of saved states
  std::stack<std::uint32_t> states_;
};

#endif  // MINIVM_MEM_DENSE_H_
