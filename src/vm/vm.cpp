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

VMOpr *VM::GetAddrById(MemId id) const {
  // find in current memory pool
  auto ptr = mems_.top().first->GetAddressById(id);
  if (!ptr) {
    // find in global memory pool
    ptr = global_mem_pool_->GetAddressById(id);
    if (!ptr) {
      LogError("invalid memory pool address");
      return nullptr;
    }
  }
  return reinterpret_cast<VMOpr *>(ptr);
}

VMOpr *VM::GetAddrBySym(SymId sym) const {
  // find in current memory pool
  auto ptr = mems_.top().first->GetAddressBySym(sym);
  if (!ptr) {
    // find in global memory pool
    ptr = global_mem_pool_->GetAddressBySym(sym);
    if (!ptr) {
      LogError("invalid memory pool address");
      return nullptr;
    }
  }
  return reinterpret_cast<VMOpr *>(ptr);
}

std::optional<MemId> VM::GetMemId(SymId sym) const {
  // find in current memory pool
  auto id = mems_.top().first->GetMemId(sym);
  if (!id) {
    // find in global memory pool
    id = global_mem_pool_->GetMemId(sym);
    if (!id) {
      LogError("symbol not found in memory pool");
      return {};
    }
  }
  return id;
}

void VM::InitFuncCall() {
  // create new memory pool
  auto pool = mem_pool_fact_();
  // push parameters
  while (!oprs_.empty()) {
    // get symbol id of parameter
    auto sym = sym_pool_.LogId("p" + std::to_string(oprs_.size() - 1));
    // allocate new memory
    auto ret = pool->Allocate(sym, 4);
    static_cast<void>(ret);
    assert(ret);
    // get pointer to memory
    auto ptr = pool->GetAddressBySym(sym);
    assert(ptr);
    // write value
    *reinterpret_cast<VMOpr *>(ptr) = oprs_.top();
    oprs_.pop();
  }
  // add memory pool & return address to stack
  mems_.push({std::move(pool), pc_ + 1});
}

void VM::Reset() {
  // reset pc to zero
  pc_ = 0;
  // clear all stacks
  while (!oprs_.empty()) oprs_.pop();
  while (!mems_.empty()) mems_.pop();
  // make a new memory pool for global environment
  auto pool = mem_pool_fact_();
  global_mem_pool_ = pool.get();
  mems_.push({std::move(pool), 0});
  // clear all static registers
  regs_.assign(regs_.size(), 0);
}

bool VM::RegisteFunction(std::string_view name, ExtFunc func) {
  auto id = sym_pool_.LogId(name);
  return ext_funcs_.insert({id, func}).second;
}

std::optional<VMOpr> VM::Run() {
#define VM_NEXT(pc_ofs)                            \
  do {                                             \
    pc_ += pc_ofs;                                 \
    inst = cont_.insts().data() + pc_;             \
    goto *kInstLabels[static_cast<int>(inst->op)]; \
  } while (0)

  const void *kInstLabels[] = {VM_INSTS(VM_EXPAND_LABEL_LIST)};
  const VMInst *inst = cont_.insts().data();
  goto *kInstLabels[static_cast<int>(inst->op)];

  // allocate memory for variable
  VM_LABEL(Var) {
    auto ret = mems_.top().first->Allocate(inst->opr, 4);
    static_cast<void>(ret);
    assert(ret);
    VM_NEXT(1);
  }

  // allocate memory for array
  VM_LABEL(Arr) {
    // get size of array
    std::uint32_t size = PopValue();
    // allocate a new memory
    auto ret = mems_.top().first->Allocate(inst->opr, size);
    static_cast<void>(ret);
    assert(ret);
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

  // load address of symbol
  VM_LABEL(LdAddr) {
    // get offset
    auto offset = PopValue();
    // get memory pool id of symbol
    auto id = GetMemId(inst->opr);
    if (!id) return {};
    // push address (id with offset) to stack
    oprs_.push(*id + offset);
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
    *ptr = oprs_.top();
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
    regs_[inst->opr] = oprs_.top();
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
    oprs_.top() &= kMaskLo;
    oprs_.top() |= (inst->opr & kMaskHi) << kVMInstImmLen;
    VM_NEXT(1);
  }

  // branch if not zero
  VM_LABEL(Bnz) {
    // get condition
    auto cond = oprs_.top();
    oprs_.pop();
    // check & jump
    if (cond) {
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
    auto addr_ofs = mems_.top().second - pc_;
    mems_.pop();
    if (mems_.empty()) {
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
        it->second(*this);
      }
    }
    VM_NEXT(1);
  }

  // logical negation
  VM_LABEL(LNot) {
    oprs_.top() = !oprs_.top();
    VM_NEXT(1);
  }

  // logical AND
  VM_LABEL(LAnd) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() && rhs;
    VM_NEXT(1);
  }

  // logical OR
  VM_LABEL(LOr) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() || rhs;
    VM_NEXT(1);
  }

  // set if equal
  VM_LABEL(Eq) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() == rhs;
    VM_NEXT(1);
  }

  // set if not equal
  VM_LABEL(Ne) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() != rhs;
    VM_NEXT(1);
  }

  // set if greater than
  VM_LABEL(Gt) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() > rhs;
    VM_NEXT(1);
  }

  // set if less than
  VM_LABEL(Lt) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() < rhs;
    VM_NEXT(1);
  }

  // set if greater than or equal
  VM_LABEL(Ge) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() >= rhs;
    VM_NEXT(1);
  }

  // set if less than or equal
  VM_LABEL(Le) {
    auto rhs = PopValue();
    oprs_.top() = oprs_.top() <= rhs;
    VM_NEXT(1);
  }

  // negation
  VM_LABEL(Neg) {
    oprs_.top() = -oprs_.top();
    VM_NEXT(1);
  }

  // addition
  VM_LABEL(Add) {
    auto rhs = PopValue();
    oprs_.top() += rhs;
    VM_NEXT(1);
  }

  // subtraction
  VM_LABEL(Sub) {
    auto rhs = PopValue();
    oprs_.top() -= rhs;
    VM_NEXT(1);
  }

  // multiplication
  VM_LABEL(Mul) {
    auto rhs = PopValue();
    oprs_.top() *= rhs;
    VM_NEXT(1);
  }

  // division
  VM_LABEL(Div) {
    auto rhs = PopValue();
    oprs_.top() /= rhs;
    VM_NEXT(1);
  }

  // modulo operation
  VM_LABEL(Mod) {
    auto rhs = PopValue();
    oprs_.top() %= rhs;
    VM_NEXT(1);
  }

#undef VM_NEXT
}
