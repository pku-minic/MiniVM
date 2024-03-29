#include "back/codegen.h"

#include <iostream>
#include <cassert>
#include <cstddef>

#include "xstl/style.h"

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
          // treat 'kVMEntry' as the entry function
          entry_pc_ = inst.opr;
        }
        else {
          // in-block label
          labels_.insert(inst.opr);
        }
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
    if (i == entry_pc_) {
      cur_func = &entry_func_;
    }
    else if (cont_.func_pcs().count(i)) {
      cur_func = &funcs_.emplace_back(i, FuncBody()).second;
    }
    // push instruction to the current function
    cur_func->push_back(cont_.insts()[i]);
  }
}

void CodeGenerator::LogError(std::string_view message, vm::VMAddr pc) {
  using namespace xstl;
  std::cerr << style("Br") << "error ";
  // try to get line number
  auto line_num = cont_.FindLineNum(pc);
  if (line_num) {
    std::cerr << style("B") << "(line " << *line_num << "): ";
  }
  else {
    std::cerr << style("B") << "(pc " << pc << "): ";
  }
  // print error message
  std::cerr << message << std::endl;
  has_error_ = true;
}

void CodeGenerator::Generate() {
  // reset internal state
  has_error_ = false;
  labels_.clear();
  funcs_.clear();
  entry_func_.clear();
  Reset();
  // generate labels & functions
  CollectLabelInfo();
  BuildFunctions();
  // generate on all functions
  for (const auto &[pc, func] : funcs_) {
    GenerateOnFunc(pc, func);
  }
  GenerateOnEntry(entry_pc_, entry_func_);
}
