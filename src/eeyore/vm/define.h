#ifndef MINIVM_EEYORE_VM_DEFINE_H_
#define MINIVM_EEYORE_VM_DEFINE_H_

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <vector>
#include <utility>
#include <ostream>
#include <unordered_set>
#include <map>
#include <cstdint>

#include "mem/pool.h"

// all supported VM instructions
#define VM_INSTS(e)                                                     \
  /* symbol definition */                                               \
  e(Var)                                                                \
  /* load & store */                                                    \
  e(Load) e(Store) e(Imm)                                               \
  /* branching (with absolute target address) */                        \
  e(Bnz) e(Jmp)                                                         \
  /* function call related stuffs                                       \
     with absolute target address or symbol name (external function) */ \
  e(Call) e(CallExt) e(Ret) e(Param)                                    \
  /* logical operations */                                              \
  e(LNot) e(LAnd) e(LOr)                                                \
  /* comparisons */                                                     \
  e(Eq) e(Ne) e(Gt) e(Lt) e(Ge) e(Le)                                   \
  /* arithmetic operations */                                           \
  e(Neg) e(Add) e(Sub) e(Mul) e(Div) e(Mod)
// expand macro to comma-separated list
#define VM_EXPAND_LIST(i)         i,
// expand macro to comma-separated string array
#define VM_EXPAND_STR_ARRAY(i)    #i,
// expand macro to label list
#define VM_EXPAND_LABEL_LIST(i)   &&VML_##i,
// define a label of VM threading
#define VM_LABEL(l)               VML_##l:

// opcode of VM instructions
// NOTE: length up to 8 bit, although it's declared as 'std::uint32_t'
enum class InstOp : std::uint32_t { VM_INSTS(VM_EXPAND_LIST) };

// VM instruction (packed, 64-bit long)
struct VMInst {
  // opcode
  InstOp op : 8;
  // symbol reference
  std::uint32_t sym : 24;
  // immediate
  std::int32_t imm : 32;
};

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

// container for storing VM instructions
class VMInstContainer {
 public:
  VMInstContainer(SymbolPool &sym_pool)
      : sym_pool_(sym_pool), has_error_(false), cur_env_(&global_env_) {}

  // instruction generators
  // exit if any errors occur
  void PushVar(std::string_view name);
  void PushVar(std::string_view name, std::uint32_t size);
  void PushLabel(std::string_view name);
  void PushLoad(std::string_view sym);
  void PushLoad(std::string_view sym, std::uint32_t offset);
  void PushLoadImm(std::int32_t imm);
  void PushStore(std::string_view sym);
  void PushStore(std::string_view sym, std::uint32_t offset);
  void PushBnz(std::string_view label);
  void PushJump(std::string_view label);
  void PushCall(std::string_view label);
  void PushOp(InstOp op);

  // update line definition
  // should be used before instruction insertion
  void LogLineNum(std::uint32_t line_num);
  // enter function environment
  void EnterFunc(std::uint32_t param_count);
  // exit function environment
  void ExitFunc();
  // perform label backfilling, and seal current container
  // exit if any errors occur
  void SealContainer();

  // dump all stored instructions
  void Dump(std::ostream &os) const;
  // query pc by line number
  std::optional<std::uint32_t> FindPC(std::uint32_t line_num) const;
  // query pc by label
  std::optional<std::uint32_t> FindPC(std::string_view label) const;
  // query line number by pc
  std::optional<std::uint32_t> FindLineNum(std::uint32_t pc) const;

  // getters
  const std::vector<VMInst> &insts() const { return insts_; }

 private:
  struct BackfillInfo {
    // indicates current label has already been defined
    bool defined;
    // pc of current label
    std::uint32_t pc;
    // pc of all instructions that related to current label
    std::vector<std::uint32_t> related_insts;
  };

  // print error message to stderr
  void LogError(std::string_view message);
  void LogError(std::string_view message, std::string_view sym);
  void LogError(std::string_view message, std::string_view sym,
                std::uint32_t line_num);
  // define new symbol, and check for conflict
  std::uint32_t DefSymbol(std::string_view sym);
  // check if symbol has not been defined, and get symbol id
  std::uint32_t GetSymbol(std::string_view sym);
  // add next pc address to backfill list
  // should be used before instruction insertion
  void LogRelatedInsts(std::string_view label);

  // symbol pool
  SymbolPool &sym_pool_;
  // error occurred during instruction generation
  bool has_error_;
  // current line number & function parameter count
  std::uint32_t cur_line_num_;
  // global & local & current environment
  std::unordered_set<std::uint32_t> global_env_, local_env_, *cur_env_;
  // line number corresponding to pc addresses (for debugging)
  std::unordered_map<std::uint32_t, std::uint32_t> line_defs_;
  // line number of pc addresses (for debugging)
  std::map<std::uint32_t, std::uint32_t> pc_defs_;
  // pc address of labels (for debugging & backfilling)
  std::unordered_map<std::string, BackfillInfo> label_defs_;
  // all instructions
  std::vector<VMInst> insts_;
};

#endif  // MINIVM_EEYORE_VM_DEFINE_H_
