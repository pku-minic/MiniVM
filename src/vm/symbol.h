#ifndef MINIVM_VM_SYMBOL_H_
#define MINIVM_VM_SYMBOL_H_

#include <string_view>
#include <optional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>

// symbol identifier
using SymId = std::uint32_t;

// symbol pool, storing all symbols
class SymbolPool {
 public:
  SymbolPool() {}

  // reset internal state
  void Reset();

  // query & get id of the specific symbol
  // create a new symbol if not found
  SymId LogId(std::string_view symbol);
  // query id of the specific symbol
  std::optional<SymId> FindId(std::string_view symbol) const;
  // query symbol by id
  std::optional<std::string_view> FindSymbol(SymId id) const;

 private:
  using SymbolPtr = std::unique_ptr<char[]>;

  // push a new symbol to pool
  void PushNewSymbol(std::string_view symbol);

  std::unordered_map<std::string_view, SymId> defs_;
  std::vector<SymbolPtr> pool_;
};

#endif  // MINIVM_VM_SYMBOL_H_
