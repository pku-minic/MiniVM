#ifndef MINIVM_VM_VM_H_
#define MINIVM_VM_VM_H_

#include <unordered_map>
#include <memory>
#include <utility>
#include <functional>
#include <string_view>
#include <optional>
#include <stack>
#include <vector>
#include <cstddef>

#include "vm/define.h"
#include "vm/symbol.h"
#include "vm/instcont.h"
#include "mem/pool.h"

// MiniVM instance
class VM {
 public:
  // environment
  using Environment = std::unordered_map<SymId, VMOpr>;
  using EnvPtr = std::shared_ptr<Environment>;
  // pair of environment and function return address
  using EnvAddrPair = std::pair<EnvPtr, VMAddr>;
  // static registers
  // external functions
  using ExtFunc = std::function<bool(VM &)>;

  VM(SymbolPool &sym_pool, VMInstContainer &cont)
      : sym_pool_(sym_pool), cont_(cont) {}

  // register an external function
  bool RegisterFunction(std::string_view name, ExtFunc func);
  // read the value of the parameter in current memory pool
  std::optional<VMOpr> GetParamFromCurPool(std::size_t param_id) const;

  // reset internal states
  void Reset();
  // run VM, 'Reset' method must be called before
  // returns top of stack (success) or 'nullopt' (failed)
  std::optional<VMOpr> Run();

  // setters
  // set memory pool
  void set_mem_pool(MemPoolPtr mem_pool) {
    mem_pool_ = std::move(mem_pool);
  }
  // set count of static registers
  void set_static_reg_count(std::uint32_t count) {
    regs_.clear();
    regs_.resize(count);
  }
  // set id of return value register
  void set_ret_reg_id(RegId ret_reg_id) { ret_reg_id_ = ret_reg_id; }

  // getters
  // symbol pool
  SymbolPool &sym_pool() { return sym_pool_; }
  // instruction container
  VMInstContainer &cont() { return cont_; }
  // program counter
  VMAddr pc() const { return pc_; }
  // operand stack
  std::stack<VMOpr> &oprs() { return oprs_; }
  // memory pool
  const MemPoolPtr &mem_pool() const { return mem_pool_; }
  // current environment & return address
  EnvAddrPair &env_addr_pair() { return envs_.top(); }
  // static registers
  VMOpr &regs(RegId id) { return regs_[id]; }

 private:
  // print error message to stderr
  void LogError(std::string_view message) const;
  // pop value from stack and return it
  VMOpr PopValue();
  // get reference of the top of stack
  VMOpr &GetOpr();
  // get address of memory by id
  VMOpr *GetAddrById(MemId id) const;
  // get address of memory by symbol
  VMOpr *GetAddrBySym(SymId sym) const;
  // make a new environment
  EnvPtr MakeEnv();
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
  // memory pool
  MemPoolPtr mem_pool_;
  // environment stack
  std::stack<EnvAddrPair> envs_;
  // global environment
  EnvPtr global_env_;
  // static registers
  std::vector<VMOpr> regs_;
  // id of return value register
  RegId ret_reg_id_;
  // external function table
  std::unordered_map<SymId, ExtFunc> ext_funcs_;
};

#endif  // MINIVM_VM_VM_H_
