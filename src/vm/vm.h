#ifndef MINIVM_VM_VM_H_
#define MINIVM_VM_VM_H_

#include <functional>
#include <string_view>
#include <optional>
#include <stack>
#include <utility>
#include <vector>
#include <unordered_map>

#include "vm/define.h"
#include "vm/symbol.h"
#include "vm/instcont.h"
#include "mem/pool.h"

// MiniVM instance
class VM {
 public:
  // external functions
  using ExtFunc = std::function<void(VM &)>;

  VM(SymbolPool &sym_pool, VMInstContainer &cont)
      : sym_pool_(sym_pool), cont_(cont) {}

  // reset internal state
  void Reset();
  // register an external function
  bool RegisteFunction(std::string_view name, ExtFunc func);
  // run VM, 'Reset' method must be called before
  // returns top of stack (success) or 'nullopt' (failed)
  std::optional<VMOpr> Run();

  // setters
  // set memory pool factory
  void set_mem_pool_fact(MemPoolFact mem_pool_fact) {
    mem_pool_fact_ = mem_pool_fact;
  }
  // set count of static registers
  void set_static_reg_count(std::uint32_t count) {
    regs_.clear();
    regs_.resize(count);
  }

 private:
  // pair of memory pool pointer and function return address
  using PoolAddrPair = std::pair<MemPoolPtr, VMAddr>;

  // print error message to stderr
  void LogError(std::string_view message) const;
  // pop value from stack and return it
  VMOpr PopValue();
  // get address of memory by id
  VMOpr *GetAddrById(MemId id) const;
  // get address of memory by symbol
  VMOpr *GetAddrBySym(SymId sym) const;
  // get id of memory by symbol
  std::optional<MemId> GetMemId(SymId sym) const;
  // perform initialization before function call
  void InitFuncCall();

  // symbol pool
  SymbolPool &sym_pool_;
  // instruction container
  VMInstContainer &cont_;
  // program counter
  VMAddr pc_;
  // operand stack
  std::stack<VMOpr> oprs_;
  // memory pool factory
  MemPoolFact mem_pool_fact_;
  // memory pool stack
  std::stack<PoolAddrPair> mems_;
  // global memory pool (avaliable when VM is running)
  MemPoolRef global_mem_pool_;
  // static registers
  std::vector<VMOpr> regs_;
  // external function table
  std::unordered_map<SymId, ExtFunc> ext_funcs_;
};

#endif  // MINIVM_VM_VM_H_
