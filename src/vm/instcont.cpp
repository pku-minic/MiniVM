#include "vm/instcont.h"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstdlib>

#include "xstl/style.h"

namespace {

// string of all opcodes
const char *kInstOpStr[] = {VM_INSTS(VM_EXPAND_STR_ARRAY)};

// a 'Break' instruction
const VMInst kBreakInst = {static_cast<std::uint32_t>(InstOp::Break)};

}  // namespace

void VMInstContainer::PushInst(InstOp op) {
  PushInst(op, 0);
}

void VMInstContainer::PushInst(InstOp op, std::uint32_t opr) {
  auto &insts = cur_env_ == &global_env_ ? global_insts_ : insts_;
  VMInst inst = {static_cast<std::uint32_t>(op), opr};
  insts.push_back(inst);
}

VMInst *VMInstContainer::GetLastInst() {
  // get current instruction container
  auto &insts = cur_env_ == &global_env_ ? global_insts_ : insts_;
  // try to get last label definition
  auto it = label_defs_.find(std::string(last_label_));
  if (it != label_defs_.end()) {
    // check if last label points to current pc
    // we must treat all labels as barriers to prevent over-optimization
    const auto &info = it->second;
    if (info.defined && info.pc == insts.size()) return nullptr;
  }
  return insts.empty() ? nullptr : &insts.back();
}

SymId VMInstContainer::DefSymbol(std::string_view sym) {
  auto id = sym_pool_.LogId(sym);
  if (global_env_.count(id) || !cur_env_->insert(id).second) {
    LogError("symbol has already been defined", sym);
    return -1;
  }
  return id;
}

SymId VMInstContainer::GetSymbol(std::string_view sym) {
  auto id = sym_pool_.FindId(sym);
  if (!id || (!cur_env_->count(*id) && !global_env_.count(*id))) {
    LogError("using undefined symbol", sym);
    return -1;
  }
  return *id;
}

void VMInstContainer::LogRelatedInsts(std::string_view label) {
  if (cur_env_ == &global_env_) {
    return LogError("using label reference in global environment");
  }
  auto &info = label_defs_[std::string(label)];
  info.related_insts.push_back(insts_.size());
}

void VMInstContainer::Reset(std::string_view src_file) {
  sym_pool_.Reset();
  has_error_ = false;
  global_env_.clear();
  local_env_.clear();
  src_file_ = src_file;
  line_defs_.clear();
  pc_defs_.clear();
  label_defs_.clear();
  insts_.clear();
  global_insts_.clear();
  breakpoints_.clear();
  trap_mode_ = false;
  while (!step_counters_.empty()) step_counters_.pop();
  // insert jump instruction to entry point
  cur_env_ = &local_env_;
  LogRelatedInsts(kVMEntry);
  PushInst(InstOp::Jmp);
  cur_env_ = &global_env_;
}

void VMInstContainer::PushVar(std::string_view sym) {
  PushInst(InstOp::Var, DefSymbol(sym));
}

void VMInstContainer::PushArr(std::string_view sym) {
  PushInst(InstOp::Arr, DefSymbol(sym));
}

void VMInstContainer::PushLabel(std::string_view name) {
  // try to insert a new entry
  auto it = label_defs_.insert({std::string(name), {false}}).first;
  auto &info = it->second;
  // check if label has already been defined
  if (info.defined) {
    LogError("label has already been defined", name);
  }
  else {
    info.defined = true;
    info.pc = insts_.size();
    last_label_ = it->first;
  }
}

void VMInstContainer::PushLoad() {
  PushInst(InstOp::Ld);
}

void VMInstContainer::PushLoad(std::string_view sym) {
  auto sym_id = GetSymbol(sym);
  // check if last instruction is 'StVar sym'
  /* NOTE:
   *  the 'StVarP sym' instruction can not be rewritten,
   *  consider the following Eeyore statement:
   *    t0 = t0 + t0
   */
  if (auto last = GetLastInst();
      last && last->op == static_cast<std::uint32_t>(InstOp::StVar)) {
    // check if is the same symbol
    if (last->opr == sym_id) {
      // just rewrite last instruction as 'StVarP'
      last->op = static_cast<std::uint32_t>(InstOp::StVarP);
      return;
    }
  }
  PushInst(InstOp::LdVar, sym_id);
}

void VMInstContainer::PushLoad(VMOpr imm) {
  constexpr auto kLower = -(1 << (kVMInstImmLen - 1));
  constexpr auto kUpper = (1 << (kVMInstImmLen - 1)) - 1;
  constexpr auto kLowerMask = (1u << kVMInstImmLen) - 1;
  constexpr auto kUpperMask = (1u << (32 - kVMInstImmLen)) - 1;
  if (imm >= kLower && imm <= kUpper) {
    PushInst(InstOp::Imm, imm);
  }
  else {
    PushInst(InstOp::Imm, imm & kLowerMask);
    PushInst(InstOp::ImmHi, (imm >> kVMInstImmLen) & kUpperMask);
  }
}

void VMInstContainer::PushLdReg(RegId reg_id) {
  // check if last instruction is 'StReg reg_id'
  /* NOTE:
   *  the 'StRegP sym' instruction can not be rewritten,
   *  consider the following Eeyore statement:
   *    t0 = t0 + t0
   */
  if (auto last = GetLastInst();
      last && last->op == static_cast<std::uint32_t>(InstOp::StReg)) {
    // check if is the same register id
    if (last->opr == reg_id) {
      // just rewrite last instruction as 'StRegP'
      last->op = static_cast<std::uint32_t>(InstOp::StRegP);
      return;
    }
  }
  PushInst(InstOp::LdReg, reg_id);
}

void VMInstContainer::PushLdFrame(VMOpr offset) {
  PushLdFrameAddr(offset);
  PushLoad();
}

void VMInstContainer::PushLdFrameAddr(VMOpr offset) {
  PushLoad(offset * 4);
  PushLoad(kVMFrame);
  PushOp(InstOp::Add);
}

void VMInstContainer::PushStore() {
  PushInst(InstOp::St);
}

void VMInstContainer::PushStore(std::string_view sym) {
  PushInst(InstOp::StVar, GetSymbol(sym));
}

void VMInstContainer::PushStReg(RegId reg_id) {
  PushInst(InstOp::StReg, reg_id);
}

void VMInstContainer::PushStFrame(VMOpr offset) {
  PushLdFrameAddr(offset);
  PushStore();
}

void VMInstContainer::PushBnz(std::string_view label) {
  LogRelatedInsts(label);
  PushInst(InstOp::Bnz);
}

void VMInstContainer::PushJump(std::string_view label) {
  LogRelatedInsts(label);
  PushInst(InstOp::Jmp);
}

void VMInstContainer::PushCall(std::string_view label) {
  LogRelatedInsts(label);
  PushInst(InstOp::Call);
}

void VMInstContainer::PushOp(InstOp op) {
  PushInst(op);
}

void VMInstContainer::LogError(std::string_view message) {
  using namespace xstl;
  std::cerr << style("Br") << "error ";
  std::cerr << style("B") << "(line " << cur_line_num_ << "): ";
  std::cerr << message << std::endl;
  has_error_ = true;
}

void VMInstContainer::LogError(std::string_view message,
                               std::string_view sym) {
  LogError(message, sym, cur_line_num_);
}

void VMInstContainer::LogError(std::string_view message,
                               std::string_view sym,
                               std::uint32_t line_num) {
  using namespace xstl;
  std::cerr << style("Br") << "error ";
  std::cerr << style("B") << "(line " << line_num
            << ", sym \"" << sym << "\"): ";
  std::cerr << message << std::endl;
  has_error_ = true;
}

void VMInstContainer::LogLineNum(std::uint32_t line_num) {
  cur_line_num_ = line_num;
  // store local line number definitions only
  if (cur_env_ == &global_env_) return;
  line_defs_[line_num] = insts_.size();
  pc_defs_[insts_.size()] = line_num;
}

void VMInstContainer::EnterFunc(std::uint32_t param_count) {
  // switch environment
  if (cur_env_ != &global_env_) {
    return LogError("nested function is unsupported");
  }
  cur_env_ = &local_env_;
  // initialize parameters
  for (std::uint32_t i = 0; i < param_count; ++i) {
    auto param = "p" + std::to_string(i);
    DefSymbol(param);
  }
}

void VMInstContainer::EnterFunc(std::uint32_t param_count,
                                std::uint32_t slot_count) {
  EnterFunc(param_count);
  // create stack frame
  PushLoad(slot_count * 4);
  PushArr(kVMFrame);
}

void VMInstContainer::ExitFunc() {
  local_env_.clear();
  cur_env_ = &global_env_;
}

void VMInstContainer::SealContainer() {
  // insert label for entry point
  PushLabel(kVMEntry);
  // insert all global instructions
  insts_.insert(insts_.end(), global_insts_.begin(), global_insts_.end());
  global_insts_.clear();
  // insert main function call & return
  cur_env_ = &local_env_;
  PushCall(kVMMain);
  PushOp(InstOp::Ret);
  // traverse all label definitions
  for (auto it = label_defs_.begin(); it != label_defs_.end();) {
    auto &&[label, info] = *it;
    if (info.defined) {
      // backfill pc to 'imm' field of all related instructions
      for (const auto &pc : info.related_insts) {
        insts_[pc].opr = info.pc;
      }
      info.related_insts.clear();
      ++it;
    }
    else {
      // check to see if there are any non-function call instructions
      for (const auto &pc : info.related_insts) {
        auto &inst = insts_[pc];
        if (inst.op == static_cast<std::uint32_t>(InstOp::Call)) {
          // function call found, convert to external function call
          inst.op = static_cast<std::uint32_t>(InstOp::CallExt);
          inst.opr = sym_pool_.LogId(label);
        }
        else {
          // current label is indeed undefined
          auto line_num = FindLineNum(pc);
          assert(line_num);
          LogError("using undefined label", label, *line_num);
        }
      }
      // remove current label definition
      it = label_defs_.erase(it);
    }
  }
  // exit if error occurred
  if (has_error_) std::exit(-1);
  // release resources
  global_env_.clear();
  local_env_.clear();
}

void VMInstContainer::ToggleBreakpoint(VMAddr pc, bool enable) {
  if (enable) {
    // set breakpoint
    breakpoints_[pc] = insts_[pc].op;
    insts_[pc].op = static_cast<std::uint32_t>(InstOp::Break);
  }
  else {
    // remove breakpoint
    auto it = breakpoints_.find(pc);
    if (it != breakpoints_.end()) {
      insts_[pc].op = it->second;
      breakpoints_.erase(it);
    }
  }
}

void VMInstContainer::AddStepCounter(std::size_t n, StepCallback callback) {
  step_counters_.push({n, callback});
}

bool VMInstContainer::Dump(std::ostream &os, VMAddr pc) const {
  if (pc >= insts_.size()) return false;
  // get the actual instruction
  auto inst = insts_[pc];
  auto it = breakpoints_.find(pc);
  if (it != breakpoints_.end()) inst.op = it->second;
  // dump instruction
  os << kInstOpStr[static_cast<int>(inst.op)] << '\t';
  // NOTE: the order of 'case' statements is important
  switch (static_cast<InstOp>(inst.op)) {
    case InstOp::Var: case InstOp::Arr: case InstOp::LdVar:
    case InstOp::StVar: case InstOp::StVarP: case InstOp::CallExt: {
      // dump as 'sym'
      auto sym = sym_pool_.FindSymbol(inst.opr);
      assert(sym);
      os << *sym;
      break;
    }
    case InstOp::LdReg: case InstOp::StReg: case InstOp::StRegP:
    case InstOp::Imm: case InstOp::ImmHi: case InstOp::Bnz:
    case InstOp::Jmp: case InstOp::Call: {
      // dump 'opr' field directly
      os << inst.opr;
      break;
    }
    default:;
  }
  return true;
}

void VMInstContainer::Dump(std::ostream &os) const {
  for (VMAddr pc = 0; pc < insts_.size(); ++pc) {
    os << pc << ":\t";
    Dump(os, pc);
    os << std::endl;
  }
}

std::optional<VMAddr> VMInstContainer::FindPC(
    std::uint32_t line_num) const {
  auto it = line_defs_.find(line_num);
  if (it != line_defs_.end()) return it->second;
  return {};
}

std::optional<VMAddr> VMInstContainer::FindPC(
    std::string_view label) const {
  auto it = label_defs_.find(std::string(label));
  if (it != label_defs_.end()) return it->second.pc;
  return {};
}

std::optional<std::uint32_t> VMInstContainer::FindLineNum(
    VMAddr pc) const {
  auto entry_pc = FindPC(kVMEntry);
  assert(entry_pc);
  if (pc >= *entry_pc) return {};
  auto it = pc_defs_.lower_bound(pc);
  if (it == pc_defs_.end()) return {};
  return it->second;
}

const VMInst *VMInstContainer::GetInst(VMAddr pc) {
  auto inst = insts_.data() + pc;
  bool break_flag = false;
  // handle step counters
  if (!step_counters_.empty()) {
    auto size = step_counters_.size();
    while (size--) {
      auto &&[n, callback] = step_counters_.front();
      bool discard = false;
      if (!n) {
        discard = true;
        // call the callback, or return 'Break' instruction
        if (callback) {
          callback(*this);
        }
        else {
          break_flag = true;
        }
      }
      else {
        // decrease the current step counter
        --n;
      }
      // discard or push to queue again
      if (!discard) step_counters_.push(step_counters_.front());
      step_counters_.pop();
    }
  }
  return trap_mode_ || break_flag ? &kBreakInst : inst;
}
