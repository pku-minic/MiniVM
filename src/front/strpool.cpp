#include "front/strpool.h"

#include <string>
#include <unordered_set>

namespace {

std::unordered_set<std::string> strs;

}  // namespace

const char *minivm::front::NewStr(const char *str) {
  return strs.insert(str).first->c_str();
}

void minivm::front::FreeAllStrs() {
  strs.clear();
}
