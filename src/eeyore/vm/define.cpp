#include "vm/define.h"

#include <iostream>
#include <cassert>
#include <cstdlib>

#include "xstl/style.h"

namespace {

// string of all opcodes
const char *kInstOpStr[] = {VM_INSTS(VM_EXPAND_STR_ARRAY)};

}  // namespace

std::uint32_t SymbolPool::LogId(std::string_view symbol) {
  // try to find symbol
  auto it = defs_.find(symbol);
  if (it != defs_.end()) {
    return it->second;
  }
  else {
    // store to pool
    std::uint32_t id = pool_.size();
    pool_.emplace_back(symbol);
    // update symbol definition
    defs_[pool_.back()] = id;
  }
}

std::optional<std::uint32_t> SymbolPool::FindId(
    std::string_view symbol) const {
  auto it = defs_.find(symbol);
  if (it != defs_.end()) return it->second;
  return {};
}

std::optional<std::string_view> SymbolPool::FindSymbol(
    std::uint32_t id) const {
  if (id < pool_.size()) return pool_[id];
  return {};
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

std::uint32_t VMInstContainer::DefSymbol(std::string_view sym) {
  auto id = sym_pool_.LogId(sym);
  if (global_env_.count(id) || !cur_env_->insert(id).second) {
    LogError("symbol has already been defined", sym);
    return -1;
  }
  return id;
}

std::uint32_t VMInstContainer::GetSymbol(std::string_view sym) {
  auto id = sym_pool_.FindId(sym);
  if (!id || !cur_env_->count(*id) || !global_env_.count(*id)) {
    LogError("using undefined symbol", sym);
    return -1;
  }
  return *id;
}

void VMInstContainer::LogRelatedInsts(std::string_view label) {
  auto &info = label_defs_[std::string(label)];
  info.related_insts.push_back(insts_.size());
}

void VMInstContainer::PushVar(std::string_view name) {
  PushVar(name, 4);
}

void VMInstContainer::PushVar(std::string_view name, std::uint32_t size) {
  insts_.push_back({InstOp::Var, DefSymbol(name), size});
}

void VMInstContainer::PushLabel(std::string_view name) {
  auto &info = label_defs_[std::string(name)];
  // check if label has already been defined
  if (info.defined) {
    LogError("label has already been defined", name);
  }
  else {
    info.defined = true;
    info.pc = insts_.size();
  }
}

void VMInstContainer::PushLoad(std::string_view sym) {
  PushLoad(sym, 0);
}

void VMInstContainer::PushLoad(std::string_view sym,
                               std::uint32_t offset) {
  insts_.push_back({InstOp::Load, GetSymbol(sym), offset});
}

void VMInstContainer::PushLoadImm(std::int32_t imm) {
  insts_.push_back({InstOp::Imm, 0, imm});
}

void VMInstContainer::PushStore(std::string_view sym) {
  PushStore(sym, 0);
}

void VMInstContainer::PushStore(std::string_view sym,
                                std::uint32_t offset) {
  insts_.push_back({InstOp::Store, GetSymbol(sym), offset});
}

void VMInstContainer::PushBnz(std::string_view label) {
  LogRelatedInsts(label);
  insts_.push_back({InstOp::Bnz});
}

void VMInstContainer::PushJump(std::string_view label) {
  LogRelatedInsts(label);
  insts_.push_back({InstOp::Jmp});
}

void VMInstContainer::PushCall(std::string_view label) {
  LogRelatedInsts(label);
  insts_.push_back({InstOp::Call});
}

void VMInstContainer::PushOp(InstOp op) {
  insts_.push_back({op});
}

void VMInstContainer::LogLineNum(std::uint32_t line_num) {
  cur_line_num_ = line_num;
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

void VMInstContainer::ExitFunc() {
  local_env_.clear();
  cur_env_ = &global_env_;
}

void VMInstContainer::SealContainer() {
  // traverse all label definitions
  for (auto it = label_defs_.begin(); it != label_defs_.end();) {
    auto &&[label, info] = *it;
    if (info.defined) {
      // backfill pc to 'imm' field of all related instructions
      for (const auto &pc : info.related_insts) {
        insts_[pc].imm = info.pc;
      }
      info.related_insts.clear();
      ++it;
    }
    else {
      // check to see if there are any non-function call instructions
      for (const auto &pc : info.related_insts) {
        auto &inst = insts_[pc];
        if (inst.op == InstOp::Call) {
          // function call found, convert to external function call
          inst.op = InstOp::CallExt;
          inst.sym = sym_pool_.LogId(label);
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
}

void VMInstContainer::Dump(std::ostream &os) const {
  for (std::uint32_t pc = 0; pc < insts_.size(); ++pc) {
    const auto &inst = insts_[pc];
    os << pc << ":\t" << kInstOpStr[static_cast<int>(inst.op)] << '\t';
    // NOTE: the order of 'case' statements is important
    switch (inst.op) {
      case InstOp::Var: case InstOp::Load: case InstOp::Store:
      case InstOp::CallExt: {
        // dump 'sym' field
        auto sym = sym_pool_.FindSymbol(inst.sym);
        assert(sym);
        os << *sym;
        if (inst.op != InstOp::CallExt) os << ", ";
        // fall through
      }
      case InstOp::Imm: case InstOp::Bnz: case InstOp::Jmp:
      case InstOp::Call: {
        // dump 'imm' field
        if (inst.op != InstOp::CallExt) os << inst.imm;
        break;
      }
      default:;
    }
    os << std::endl;
  }
}

std::optional<std::uint32_t> VMInstContainer::FindPC(
    std::uint32_t line_num) const {
  auto it = line_defs_.find(line_num);
  if (it != line_defs_.end()) return it->second;
  return {};
}

std::optional<std::uint32_t> VMInstContainer::FindPC(
    std::string_view label) const {
  auto it = label_defs_.find(std::string(label));
  if (it != label_defs_.end()) return it->second.pc;
  return {};
}

std::optional<std::uint32_t> VMInstContainer::FindLineNum(
    std::uint32_t pc) const {
  auto it = pc_defs_.lower_bound(pc);
  if (it == pc_defs_.end()) return {};
  assert(it != pc_defs_.begin());
  return (--it)->second;
}
