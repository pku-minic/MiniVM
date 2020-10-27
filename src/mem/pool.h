#ifndef MINIVM_MEM_POOL_H_
#define MINIVM_MEM_POOL_H_

#include <memory>
#include <functional>
#include <cstdint>

// interface of memory pool
class MemoryPoolInterface {
 public:
  virtual ~MemoryPoolInterface() = default;

  // allocate a new memory with the specific size
  // returns the id of allocated memory
  virtual std::uint32_t Allocate(std::uint32_t size) = 0;

  // get the base address of the specific memory
  // returns 'nullptr' if failed
  virtual void *GetAddress(std::uint32_t id) const = 0;
};

// pointer to memory pool
using MemPoolPtr = std::unique_ptr<MemoryPoolInterface>;
// factory function of memory pool
using MemPoolFact = std::function<MemPoolPtr()>;

#endif  // MINIVM_MEM_POOL_H_
