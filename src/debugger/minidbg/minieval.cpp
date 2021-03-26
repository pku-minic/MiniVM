#include "debugger/minidbg/minieval.h"

#include "front/token.h"

namespace {

// name of static registers
constexpr std::string_view kRegNames[] = {
    TOKEN_REGISTERS(TOKEN_EXPAND_SECOND)
};

}  // namespace

std::optional<VMOpr> MiniEvaluator::GetSymVal(std::string_view sym) {
  // try to get symbol id
  auto id = vm_.sym_pool().FindId(sym);
  if (!id) return {};
  // try to find in current environment
  const auto &env = vm_.env_addr_pair().first;
  auto it = env->find(*id);
  if (it != env->end()) return it->second;
  // try to find in global environment
  it = vm_.global_env()->find(*id);
  if (it != vm_.global_env()->end()) return it->second;
  return {};
}

std::optional<VMOpr> MiniEvaluator::GetRegVal(std::string_view reg) {
  if (reg == "pc") {
    return vm_.pc();
  }
  else {
    for (RegId i = 0; i < TOKEN_COUNT(TOKEN_REGISTERS); ++i) {
      if (kRegNames[i] == reg) return vm_.regs(i);
    }
    return {};
  }
}

std::optional<VMOpr> MiniEvaluator::GetValueOfSym(std::string_view sym) {
  return sym[0] == '$' ? GetRegVal(sym.substr(1)) : GetSymVal(sym);
}

std::optional<VMOpr> MiniEvaluator::GetValueOfAddr(VMOpr addr) {
  // try to find in memory pool
  auto ptr = vm_.mem_pool()->GetAddress(addr);
  if (!ptr) return {};
  return *reinterpret_cast<VMOpr *>(ptr);
}
