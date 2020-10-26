#include "front/strpool.h"

#include <string>
#include <unordered_set>

namespace {

std::unordered_set<std::string> strs;

}  // namespace

const char *NewStr(const char *str) {
  return strs.insert(str).first->c_str();
}

void FreeAllStrs() {
  strs.clear();
}
