#ifndef MINIVM_DEBUGGER_MINIDBG_MINIEVAL_H_
#define MINIVM_DEBUGGER_MINIDBG_MINIEVAL_H_

#include "debugger/expreval.h"
#include "vm/vm.h"

// expression evaluator for MiniVM
class MiniEvaluator : public ExprEvaluatorBase<VMOpr> {
 public:
  MiniEvaluator(VM &vm) : vm_(vm) {}

 protected:
  std::optional<VMOpr> GetValueOfSym(std::string_view sym) override;
  std::optional<VMOpr> GetValueOfAddr(VMOpr addr) override;

 private:
  // get value of symbol in environment
  std::optional<VMOpr> GetSymVal(std::string_view sym);
  // get value of static registers or PC
  std::optional<VMOpr> GetRegVal(std::string_view reg);

  // current MiniVM instance
  VM &vm_;
};

#endif  // MINIVM_DEBUGGER_MINIDBG_MINIEVAL_H_
