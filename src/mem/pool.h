#ifndef MINIVM_MEM_POOL_H_
#define MINIVM_MEM_POOL_H_

#include <memory>
#include <cstdint>

// type of memory id
using MemId = std::uint32_t;

// interface of memory pool
class MemoryPoolInterface {
 public:
  virtual ~MemoryPoolInterface() = default;

  // allocate a new memory with the specific size
  // returns memory id
  virtual MemId Allocate(std::uint32_t size, bool init) = 0;
  // get the memory base address of the specific memory id
  // returns 'nullptr' if failed
  virtual void *GetAddress(MemId id) = 0;

  // memory pool state manipulations
  // handle size of allocated memory only
  // don't care about the contents of memory
  //
  // save current state
  virtual void SaveState() = 0;
  // restore the previous state
  virtual void RestoreState() = 0;
};

// pointer to memory pool
using MemPoolPtr = std::unique_ptr<MemoryPoolInterface>;

#endif  // MINIVM_MEM_POOL_H_
