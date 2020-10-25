#ifndef MINIVM_VM_SYMBOL_H_
#define MINIVM_VM_SYMBOL_H_

#include <string_view>
#include <string>
#include <optional>
#include <unordered_map>
#include <vector>
#include <cstdint>

// symbol pool, storing all symbols
class SymbolPool {
 public:
  SymbolPool() {}

  // query & get id of the specific symbol
  // create a new symbol if not found
  std::uint32_t LogId(std::string_view symbol);
  // query id of the specific symbol
  std::optional<std::uint32_t> FindId(std::string_view symbol) const;
  // query symbol by id
  std::optional<std::string_view> FindSymbol(std::uint32_t id) const;

 private:
  std::unordered_map<std::string_view, std::uint32_t> defs_;
  std::vector<std::string> pool_;
};

#endif  // MINIVM_VM_SYMBOL_H_
