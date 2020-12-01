#include "vm/vm.h"

#include <iostream>
#include <string>
#include <cassert>

#include "xstl/style.h"

void VM::LogError(std::string_view message) const {
  using namespace xstl;
  std::cerr << style("Br") << "error";
  if (auto line_num = cont_.FindLineNum(pc_)) {
    std::cerr << style("B") << " (line " << *line_num
              << ", pc " << pc_ << ')';
  }
  std::cerr << ": " << message << std::endl;
}

VMOpr VM::PopValue() {
  assert(!oprs_.empty());
  auto ret = oprs_.top();
  oprs_.pop();
  return ret;
}

VMOpr &VM::GetOpr() {
  assert(!oprs_.empty());
  return oprs_.top();
}

VMOpr *VM::GetAddrById(MemId id) const {
  // find in memory pool
  auto ptr = mem_pool_->GetAddress(id);
  if (!ptr) {
    LogError("invalid memory pool address");
    return nullptr;
  }
  return reinterpret_cast<VMOpr *>(ptr);
}

VMOpr *VM::GetAddrBySym(SymId sym) const {
  // find in current environment
  const auto &env = envs_.top().first;
  auto it = env->find(sym);
  if (it == env->end()) {
    // find in global environment
    it = global_env_->find(sym);
    if (it == global_env_->end()) {
      LogError("symbol not found");
      return nullptr;
    }
  }
  return &it->second;
}

VM::EnvPtr VM::MakeEnv() {
  return std::make_shared<Environment>();
}

void VM::InitFuncCall() {
  // save the state of memory pool
  mem_pool_->SaveState();
  // add a new environment & return address to stack
  envs_.push({MakeEnv(), pc_ + 1});
  auto &env = envs_.top().first;
  // push parameters
  while (!oprs_.empty()) {
    // get symbol id of parameter
    auto sym = sym_pool_.LogId("p" + std::to_string(oprs_.size() - 1));
    // write value
    auto ret = env->insert({sym, PopValue()}).second;
    static_cast<void>(ret);
    assert(ret);
  }
}

bool VM::RegisterFunction(std::string_view name, ExtFunc func) {
  auto id = sym_pool_.LogId(name);
  return ext_funcs_.insert({id, func}).second;
}

std::optional<VMOpr> VM::GetParamFromCurPool(std::size_t param_id) const {
  auto sym = sym_pool_.FindId("p" + std::to_string(param_id));
  if (!sym) return {};
  const auto &env = envs_.top().first;
  auto it = env->find(*sym);
  if (it == env->end()) return {};
  return it->second;
}

void VM::Reset() {
  // reset pc to zero
  pc_ = 0;
  // clear all stacks
  while (!oprs_.empty()) oprs_.pop();
  while (!envs_.empty()) envs_.pop();
  // make a new environment for global environment
  envs_.push({MakeEnv(), 0});
  global_env_ = envs_.top().first;
  // save current state of memory pool
  mem_pool_->SaveState();
  // clear all static registers
  regs_.assign(regs_.size(), 0);
}

std::optional<VMOpr> VM::Run() {
#define VM_NEXT(pc_ofs)          \
  do {                           \
    pc_ += pc_ofs;               \
    inst = cont_.GetInst(pc_);   \
    goto *kInstLabels[inst->op]; \
  } while (0)

  const void *kInstLabels[] = {VM_INSTS(VM_EXPAND_LABEL_LIST)};
  const VMInst *inst;
  VM_NEXT(0);

  // allocate memory for variable
  VM_LABEL(Var) {
    envs_.top().first->insert({inst->opr, 0});
    VM_NEXT(1);
  }

  // allocate memory for array
  VM_LABEL(Arr) {
    // add a new entry to environment
    auto ret = envs_.top().first->insert({inst->opr, 0});
    // allocate a new memory
    if (ret.second) ret.first->second = mem_pool_->Allocate(PopValue());
    VM_NEXT(1);
  }

  // load value from address
  VM_LABEL(Ld) {
    // get address from memory pool
    auto ptr = GetAddrById(PopValue());
    if (!ptr) return {};
    // push to stack
    oprs_.push(*ptr);
    VM_NEXT(1);
  }

  // load variable
  VM_LABEL(LdVar) {
    // get address from memory pool
    auto ptr = GetAddrBySym(inst->opr);
    if (!ptr) return {};
    // push to stack
    oprs_.push(*ptr);
    VM_NEXT(1);
  }

  // load static register
  VM_LABEL(LdReg) {
    assert(inst->opr < regs_.size());
    oprs_.push(regs_[inst->opr]);
    VM_NEXT(1);
  }

  // store value to address
  VM_LABEL(St) {
    // get address from memory pool
    auto ptr = GetAddrById(PopValue());
    if (!ptr) return {};
    // write value
    *ptr = PopValue();
    VM_NEXT(1);
  }

  // store variable
  VM_LABEL(StVar) {
    // get address from memory pool
    auto ptr = GetAddrBySym(inst->opr);
    if (!ptr) return {};
    // write value
    *ptr = PopValue();
    VM_NEXT(1);
  }

  // store variable and preserve
  VM_LABEL(StVarP) {
    // get address from memory pool
    auto ptr = GetAddrBySym(inst->opr);
    if (!ptr) return {};
    // write value
    *ptr = GetOpr();
    VM_NEXT(1);
  }

  // store static register
  VM_LABEL(StReg) {
    assert(inst->opr < regs_.size());
    regs_[inst->opr] = PopValue();
    VM_NEXT(1);
  }

  // store static register and preserve
  VM_LABEL(StRegP) {
    assert(inst->opr < regs_.size());
    regs_[inst->opr] = GetOpr();
    VM_NEXT(1);
  }

  // load immediate (sign-extended)
  VM_LABEL(Imm) {
    constexpr auto kSignBit = 1u << (kVMInstImmLen - 1);
    constexpr auto kUpperOnes = (1u << (32 - kVMInstImmLen)) - 1;
    auto val = inst->opr;
    if (val & kSignBit) val |= kUpperOnes << kVMInstImmLen;
    oprs_.push(val);
    VM_NEXT(1);
  }

  // load immediate to upper bits
  VM_LABEL(ImmHi) {
    constexpr auto kMaskLo = (1u << kVMInstImmLen) - 1;
    constexpr auto kMaskHi = (1u << (32 - kVMInstImmLen)) - 1;
    GetOpr() &= kMaskLo;
    GetOpr() |= (inst->opr & kMaskHi) << kVMInstImmLen;
    VM_NEXT(1);
  }

  // branch if not zero
  VM_LABEL(Bnz) {
    if (PopValue()) {
      pc_ = inst->opr;
      VM_NEXT(0);
    }
    else {
      VM_NEXT(1);
    }
  }

  // jump to target
  VM_LABEL(Jmp) {
    pc_ = inst->opr;
    VM_NEXT(0);
  }

  // call function
  VM_LABEL(Call) {
    InitFuncCall();
    pc_ = inst->opr;
    VM_NEXT(0);
  }

  // call external function
  VM_LABEL(CallExt) {
    // get external function
    auto it = ext_funcs_.find(inst->opr);
    if (it == ext_funcs_.end()) {
      LogError("invalid external function");
      return {};
    }
    // perform function call
    InitFuncCall();
    if (!it->second(*this)) {
      LogError("error occurred during external function call");
      return {};
    }
    // perform return operation
    VM_GOTO(Ret);
  }

  // return from function call
  VM_LABEL(Ret) {
    // restore the state of memory pool
    mem_pool_->RestoreState();
    // get offset of return address
    auto addr_ofs = envs_.top().second - pc_;
    envs_.pop();
    // check if need to stop execution
    if (envs_.empty()) {
      return regs_.empty() ? PopValue() : regs_[ret_reg_id_];
    }
    VM_NEXT(addr_ofs);
  }

  // breakpoint
  VM_LABEL(Break) {
    // find debugger callback symbol
    if (auto id = sym_pool_.FindId(kVMDebugger)) {
      // find debugger callback
      auto it = ext_funcs_.find(*id);
      if (it != ext_funcs_.end()) {
        // call debugger
        if (!it->second(*this)) return 0;
      }
    }
    VM_NEXT(-1);
  }

  // logical negation
  VM_LABEL(LNot) {
    GetOpr() = !GetOpr();
    VM_NEXT(1);
  }

  // logical AND
  VM_LABEL(LAnd) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() && rhs;
    VM_NEXT(1);
  }

  // logical OR
  VM_LABEL(LOr) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() || rhs;
    VM_NEXT(1);
  }

  // set if equal
  VM_LABEL(Eq) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() == rhs;
    VM_NEXT(1);
  }

  // set if not equal
  VM_LABEL(Ne) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() != rhs;
    VM_NEXT(1);
  }

  // set if greater than
  VM_LABEL(Gt) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() > rhs;
    VM_NEXT(1);
  }

  // set if less than
  VM_LABEL(Lt) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() < rhs;
    VM_NEXT(1);
  }

  // set if greater than or equal
  VM_LABEL(Ge) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() >= rhs;
    VM_NEXT(1);
  }

  // set if less than or equal
  VM_LABEL(Le) {
    auto rhs = PopValue();
    GetOpr() = GetOpr() <= rhs;
    VM_NEXT(1);
  }

  // negation
  VM_LABEL(Neg) {
    GetOpr() = -GetOpr();
    VM_NEXT(1);
  }

  // addition
  VM_LABEL(Add) {
    auto rhs = PopValue();
    GetOpr() += rhs;
    VM_NEXT(1);
  }

  // subtraction
  VM_LABEL(Sub) {
    auto rhs = PopValue();
    GetOpr() -= rhs;
    VM_NEXT(1);
  }

  // multiplication
  VM_LABEL(Mul) {
    auto rhs = PopValue();
    GetOpr() *= rhs;
    VM_NEXT(1);
  }

  // division
  VM_LABEL(Div) {
    auto rhs = PopValue();
    GetOpr() /= rhs;
    VM_NEXT(1);
  }

  // modulo operation
  VM_LABEL(Mod) {
    auto rhs = PopValue();
    GetOpr() %= rhs;
    VM_NEXT(1);
  }

#undef VM_NEXT
}
