#include "debugger/minidbg/minieval.h"

std::optional<VMOpr> MiniEvaluator::GetValueOfSym(std::string_view sym) {
  // try to get symbol id
  auto id = vm_.sym_pool().FindId(sym);
  if (!id) return {};
  // try to find in current environment
  const auto &env = vm_.env_addr_pair().first;
  auto it = env->find(*id);
  if (it != env->end()) return it->second;
  return {};
}

std::optional<VMOpr> MiniEvaluator::GetValueOfAddr(VMOpr addr) {
  // try to find in memory pool
  auto ptr = vm_.mem_pool()->GetAddress(addr);
  if (!ptr) return {};
  return *reinterpret_cast<VMOpr *>(ptr);
}
