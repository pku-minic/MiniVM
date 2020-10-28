#ifndef MINIVM_MEM_POOL_H_
#define MINIVM_MEM_POOL_H_

#include <optional>
#include <memory>
#include <functional>
#include <cstdint>

#include "vm/define.h"

// type of memory id
using MemId = std::uint32_t;

// interface of memory pool
class MemoryPoolInterface {
 public:
  virtual ~MemoryPoolInterface() = default;

  // allocate a new memory with the specific size for a symbol
  // returns false if failed
  virtual bool Allocate(SymId sym, std::uint32_t size) = 0;
  // get id of allocated memory of the specific symbol
  virtual std::optional<MemId> GetMemId(SymId sym) const = 0;
  // get the memory base address of the specific symbol
  // returns 'nullptr' if failed
  virtual void *GetAddressBySym(SymId sym) const = 0;
  // get the memory base address of the specific memory id
  // returns 'nullptr' if failed
  virtual void *GetAddressById(MemId id) const = 0;
};

// pointer to memory pool
using MemPoolPtr = std::unique_ptr<MemoryPoolInterface>;
// reference to memory pool
using MemPoolRef = MemoryPoolInterface *;
// factory function of memory pool
using MemPoolFact = std::function<MemPoolPtr()>;

#endif  // MINIVM_MEM_POOL_H_
