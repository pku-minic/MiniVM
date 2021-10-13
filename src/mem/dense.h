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
  DenseMemoryPool() : mems_(nullptr), mem_size_(0) {}
  ~DenseMemoryPool() { FreeMems(); }

  MemId Allocate(std::uint32_t size, bool init) override;
  void *GetAddress(MemId id) override;
  void SaveState() override;
  void RestoreState() override;

 private:
  // free all allocated memories
  void FreeMems();

  // all allocated memories
  std::uint8_t *mems_;
  // size of allocated memories
  std::uint32_t mem_size_;
  // stack of saved states
  std::stack<std::uint32_t> states_;
};

#endif  // MINIVM_MEM_DENSE_H_
