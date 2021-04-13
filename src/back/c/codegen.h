#ifndef MINIVM_BACK_C_CODEGEN_H_
#define MINIVM_BACK_C_CODEGEN_H_

#include <sstream>

#include "back/codegen.h"

namespace minivm::back::c {

// C code generator
class CCodeGen : public CodeGenerator {
 public:
  void Dump(std::ostream &os) const override;

 protected:
  void Reset() override;
  void GenerateOnFunc(const FuncBody &func) override;
  void GenerateOnMain(const FuncBody &func) override;

 private:
  // string stream for storing generated C code
  std::ostringstream oss_;
};

}  // namespace minivm::back::c

#endif  // MINIVM_BACK_C_CODEGEN_H_
