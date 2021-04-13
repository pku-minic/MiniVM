#include "back/c/codegen.h"

using namespace minivm::back::c;

namespace {

//

}  // namespace

void CCodeGen::Reset() {
  // oss_.str(kCCodeHeader);
  oss_.clear();
}

void CCodeGen::GenerateOnFunc(const FuncBody &func) {
  //
}

void CCodeGen::GenerateOnMain(const FuncBody &func) {
  //
}

void CCodeGen::Dump(std::ostream &os) const {
  //
}
