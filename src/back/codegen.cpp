#include "back/codegen.h"

#include <cassert>
#include <cstddef>

using namespace minivm::back;
using namespace minivm::vm;

void CodeGenerator::CollectLabelInfo() {
  // traverse all instructions
  for (VMAddr i = 0; i < cont_.inst_count(); ++i) {
    const auto &inst = cont_.insts()[i];
    // handle by opcode
    switch (static_cast<InstOp>(inst.op)) {
      case InstOp::Bnz: {
        // in-block label
        labels_.insert(inst.opr);
        break;
      }
      case InstOp::Jmp: {
        // check if is the first instruction (Jmp kVMEntry)
        if (!i) {
          // treat 'kVMEntry' as the main function
          main_label_ = inst.opr;
        }
        else {
          // in-block label
          labels_.insert(inst.opr);
        }
        break;
      }
      case InstOp::Call: {
        // function label
        func_labels_.insert(inst.opr);
        break;
      }
      // ignore all other instructions
      default:;
    }
  }
}

void CodeGenerator::BuildFunctions() {
  FuncBody *cur_func = nullptr;
  // traverse all instructions
  // except the first instruction (Jmp kVMEntry)
  assert(static_cast<InstOp>(cont_.insts()->op) == InstOp::Jmp);
  for (VMAddr i = 1; i < cont_.inst_count(); ++i) {
    // check if is boundary of a function
    if (i == main_label_) {
      cur_func = &main_func_;
    }
    else if (func_labels_.count(i)) {
      cur_func = &funcs_.emplace_back();
    }
    // push instruction to the current function
    cur_func->push_back(cont_.insts()[i]);
  }
}

void CodeGenerator::Generate() {
  // reset internal state
  labels_.clear();
  func_labels_.clear();
  funcs_.clear();
  main_func_.clear();
  Reset();
  // generate labels & functions
  CollectLabelInfo();
  BuildFunctions();
  // generate on all functions
  for (const auto &func : funcs_) {
    GenerateOnFunc(func);
  }
  GenerateOnMain(main_func_);
}
