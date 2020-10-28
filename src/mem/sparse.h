#ifndef MINIVM_MEM_SPARSE_H_
#define MINIVM_MEM_SPARSE_H_

#include <memory>
#include <unordered_map>
#include <map>
#include <cstdint>

#include "mem/pool.h"

// sparse memory pool
// naive implementation, cause id of allocated memory can not be reused
// NOTE: thread unsafe!
class SparseMemoryPool : public MemoryPoolInterface {
 public:
  SparseMemoryPool() {}

  bool Allocate(SymId sym, std::uint32_t size) override;
  std::optional<std::uint32_t> GetMemId(SymId sym) const override;
  void *GetAddressBySym(SymId sym) const override;
  void *GetAddressById(std::uint32_t id) const override;

 private:
  using BytesPtr = std::unique_ptr<std::uint8_t[]>;

  // map of symbol id to memory id
  std::unordered_map<SymId, std::uint32_t> ids_;
  // all allocated memories
  std::map<std::uint32_t, BytesPtr> mems_;
  // size of all allocated memories (shared)
  static std::uint32_t mem_size_;
};

#endif  // MINIVM_MEM_SPARSE_H_
