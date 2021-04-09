#ifndef MINIVM_DEBUGGER_MINIDBG_MINIEVAL_H_
#define MINIVM_DEBUGGER_MINIDBG_MINIEVAL_H_

#include "debugger/expreval.h"
#include "vm/vm.h"

namespace minivm::debugger::minidbg {

// expression evaluator for MiniVM
class MiniEvaluator : public ExprEvaluatorBase<vm::VMOpr> {
 public:
  MiniEvaluator(vm::VM &vm) : vm_(vm) {}

 protected:
  std::optional<vm::VMOpr> GetValueOfSym(std::string_view sym) override;
  std::optional<vm::VMOpr> GetValueOfAddr(vm::VMOpr addr) override;

 private:
  // get value of symbol in environment
  std::optional<vm::VMOpr> GetSymVal(std::string_view sym);
  // get value of static registers or PC
  std::optional<vm::VMOpr> GetRegVal(std::string_view reg);

  // current MiniVM instance
  vm::VM &vm_;
};

}  // namespace minivm::debugger::minidbg

#endif  // MINIVM_DEBUGGER_MINIDBG_MINIEVAL_H_
