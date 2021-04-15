#ifndef MINIVM_BACK_C_CODEGEN_H_
#define MINIVM_BACK_C_CODEGEN_H_

#include <optional>
#include <string>
#include <sstream>
#include <cstdint>

#include "back/codegen.h"
#include "debugger/minidbg/srcreader.h"

namespace minivm::back::c {

// C code generator
class CCodeGen : public CodeGenerator {
 public:
  CCodeGen(const vm::VMInstContainer &cont, bool tigger_mode)
      : CodeGenerator(cont), tigger_mode_(tigger_mode),
        src_reader_(cont.src_file()) {}

  void Dump(std::ostream &os) const override;

 protected:
  void Reset() override;
  void GenerateOnFunc(vm::VMAddr pc, const FuncBody &func) override;
  void GenerateOnEntry(vm::VMAddr pc, const FuncBody &func) override;

 private:
  // get symbol name for C code generation
  std::optional<std::string> GetSymbol(vm::SymId sym_id, vm::VMAddr pc);
  // push value to stack
  // generate instruction
  bool GenerateInst(std::ostringstream &oss, vm::VMAddr pc,
                    const vm::VMInst &inst);

  // generate Tigger mode code
  bool tigger_mode_;
  // string stream for storing generated C code
  std::ostringstream global_, code_;
  // last line position
  std::uint32_t last_line_;
  // source file reader
  debugger::minidbg::SourceReader src_reader_;
};

}  // namespace minivm::back::c

#endif  // MINIVM_BACK_C_CODEGEN_H_
