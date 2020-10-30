#ifndef MINIVM_VM_INSTCONT_H_
#define MINIVM_VM_INSTCONT_H_

#include <string_view>
#include <ostream>
#include <optional>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <functional>
#include <map>
#include <cstdint>

#include "vm/define.h"
#include "vm/symbol.h"

// container for storing VM instructions
class VMInstContainer {
 public:
  VMInstContainer(SymbolPool &sym_pool)
      : sym_pool_(sym_pool), has_error_(false), cur_env_(&global_env_) {
    Reset();
  }

  // reset internal states
  void Reset();

  // instruction generators
  // exit if any error occurred
  void PushVar(std::string_view sym);
  void PushArr(std::string_view sym);
  void PushLabel(std::string_view label);
  void PushLoad();
  void PushLoad(std::string_view sym);
  void PushLoad(VMOpr imm);
  void PushLdReg(RegId reg_id);
  void PushLdFrame(VMOpr offset);
  void PushLdFrameAddr(VMOpr offset);
  void PushStore();
  void PushStore(std::string_view sym);
  void PushStReg(RegId reg_id);
  void PushStFrame(VMOpr offset);
  void PushBnz(std::string_view label);
  void PushJump(std::string_view label);
  void PushCall(std::string_view label);
  void PushOp(InstOp op);

  // print error message to stderr
  void LogError(std::string_view message);
  void LogError(std::string_view message, std::string_view sym);
  void LogError(std::string_view message, std::string_view sym,
                std::uint32_t line_num);
  // update line definition
  // should be called before instruction insertion
  void LogLineNum(std::uint32_t line_num);
  // enter function environment
  // should be called before instruction insertion
  void EnterFunc(std::uint32_t param_count);
  void EnterFunc(std::uint32_t param_count, std::uint32_t slot_count);
  // exit function environment
  void ExitFunc();
  // perform label backfilling, and seal current container
  // exit if any errors occur
  void SealContainer();

  // dump all stored instructions
  void Dump(std::ostream &os) const;
  // query pc by line number
  std::optional<VMAddr> FindPC(std::uint32_t line_num) const;
  // query pc by label
  std::optional<VMAddr> FindPC(std::string_view label) const;
  // query line number by pc
  std::optional<std::uint32_t> FindLineNum(VMAddr pc) const;

  // getters
  const std::vector<VMInst> &insts() const { return insts_; }

 private:
  struct BackfillInfo {
    // indicates current label has already been defined
    bool defined;
    // pc of current label
    VMAddr pc;
    // pc of all instructions that related to current label
    std::vector<VMAddr> related_insts;
  };

  // push instruction to container
  void PushInst(InstOp op);
  void PushInst(InstOp op, std::uint32_t opr);
  // get reference to last instruction
  // returns 'nullptr' if failed
  VMInst *GetLastInst();
  // define new symbol, and check for conflict
  SymId DefSymbol(std::string_view sym);
  // check if symbol has not been defined, and get symbol id
  SymId GetSymbol(std::string_view sym);
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
  std::unordered_set<SymId> global_env_, local_env_, *cur_env_;
  // line number corresponding to pc addresses (for debugging)
  std::unordered_map<std::uint32_t, VMAddr> line_defs_;
  // line number of pc addresses (for debugging)
  std::map<VMAddr, std::uint32_t, std::greater<VMAddr>> pc_defs_;
  // pc address of labels (for debugging & backfilling)
  std::unordered_map<std::string, BackfillInfo> label_defs_;
  // last defined label
  std::string_view last_label_;
  // all instructions
  std::vector<VMInst> insts_, global_insts_;
};

#endif  // MINIVM_VM_INSTCONT_H_
