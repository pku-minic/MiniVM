#ifndef MINIVM_MEM_DENSE_H_
#define MINIVM_MEM_DENSE_H_

#include <vector>
#include <unordered_map>
#include <cstdint>

#include "mem/pool.h"

// dense memory pool
// all memory will be allocated contiguously in one place
// NOTE:
// 1. thread unsafe!
// 2. only allow access to the most recently allocated memory pool
class DenseMemoryPool : public MemoryPoolInterface {
 public:
  DenseMemoryPool() : last_mem_size_(mems_.size()) {}
  ~DenseMemoryPool() { Release(); }

  bool Allocate(SymId sym, std::uint32_t size) override;
  std::optional<MemId> GetMemId(SymId sym) const override;
  void *GetAddressBySym(SymId sym) const override;
  void *GetAddressById(MemId id) const override;

 private:
  void Release();

  // all allocated memories
  static std::vector<std::uint8_t> mems_;
  // map of symbol id to memory id
  std::unordered_map<SymId, MemId> ids_;
  // size of all allocated memories before creating the current pool
  std::uint32_t last_mem_size_;
};

#endif  // MINIVM_MEM_DENSE_H_
