#ifndef MINIVM_BACK_CODEGEN_H_
#define MINIVM_BACK_CODEGEN_H_

#include <ostream>
#include <vector>
#include <string_view>
#include <utility>
#include <unordered_set>

#include "vm/instcont.h"

namespace minivm::back {

// code generator for Gopher
class CodeGenerator {
 public:
  CodeGenerator(const vm::VMInstContainer &cont) : cont_(cont) {}
  virtual ~CodeGenerator() = default;

  // generate code
  void Generate();

  // dump generated code to the specific stream
  virtual void Dump(std::ostream &os) const = 0;

  // getters
  bool has_error() const { return has_error_; }

 protected:
  // function body
  using FuncBody = std::vector<vm::VMInst>;

  // check if there is a label on the specific address
  bool IsLabel(vm::VMAddr addr) const { return labels_.count(addr); }
  // print error message to stderr
  void LogError(std::string_view message, vm::VMAddr pc);

  // reset internal state
  virtual void Reset() {};
  // generate code on function
  virtual void GenerateOnFunc(vm::VMAddr pc, const FuncBody &func) = 0;
  // generate code on entry function
  virtual void GenerateOnEntry(const FuncBody &func) = 0;

  // getters
  const vm::VMInstContainer &cont() const { return cont_; }

 private:
  // collect all label/function information
  void CollectLabelInfo();
  // split code into functions
  void BuildFunctions();

  // instruction container
  const vm::VMInstContainer &cont_;
  // error flag
  bool has_error_;
  // label definitions
  std::unordered_set<vm::VMAddr> labels_;
  // function labels
  std::unordered_set<vm::VMAddr> func_labels_;
  // label of entry function
  vm::VMAddr entry_label_;
  // functions
  std::vector<std::pair<vm::VMAddr, FuncBody>> funcs_;
  // entry function
  FuncBody entry_func_;
};

}  // namespace minivm::back

#endif  // MINIVM_BACK_CODEGEN_H_
