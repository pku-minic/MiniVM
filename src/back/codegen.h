#ifndef MINIVM_BACK_CODEGEN_H_
#define MINIVM_BACK_CODEGEN_H_

#include <ostream>
#include <vector>
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

 protected:
  // function body
  using FuncBody = std::vector<vm::VMInst>;

  // check if there is a label on the specific address
  bool IsLabel(vm::VMAddr addr) const { return labels_.count(addr); }

  // reset internal state
  virtual void Reset() {};
  // generate code on function
  virtual void GenerateOnFunc(const FuncBody &func) = 0;
  // generate code on main function
  virtual void GenerateOnMain(const FuncBody &func) = 0;

  // getters
  const vm::VMInstContainer &cont() const { return cont_; }

 private:
  // collect all label/function information
  void CollectLabelInfo();
  // split code into functions
  void BuildFunctions();

  // instruction container
  const vm::VMInstContainer &cont_;
  // label definitions
  std::unordered_set<vm::VMAddr> labels_;
  // function labels
  std::unordered_set<vm::VMAddr> func_labels_;
  // label of main function
  vm::VMAddr main_label_;
  // functions
  std::vector<FuncBody> funcs_;
  // main function
  FuncBody main_func_;
};

}  // namespace minivm::back

#endif  // MINIVM_BACK_CODEGEN_H_
