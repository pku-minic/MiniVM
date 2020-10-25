#ifndef MINIVM_MEM_POOL_H_
#define MINIVM_MEM_POOL_H_

#include <memory>
#include <cstdint>

// interface of memory pool
class MemoryPoolInterface {
 public:
  virtual ~MemoryPoolInterface() = default;

  // allocate a new memory area
  virtual std::uint32_t Allocate(std::uint32_t size) = 0;
  // get the base address of the specific memory area
  virtual void *GetAddress(std::uint32_t id) = 0;
};

// pointer to memory pool
using MemPoolPtr = std::unique_ptr<MemoryPoolInterface>;

#endif  // MINIVM_MEM_POOL_H_
