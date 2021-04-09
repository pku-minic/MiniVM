#ifndef MINIVM_VM_DEFINE_H_
#define MINIVM_VM_DEFINE_H_

#include <cstddef>
#include <cstdint>

// all supported VM instructions
// for more details, see `src/vm/README.md`
#define VM_INSTS(e)                                     \
  /* memory allocation */                               \
  e(Var) e(Arr)                                         \
  /* load & store */                                    \
  e(Ld) e(LdVar) e(LdReg) e(St) e(StVar) e(StVarP)      \
  e(StReg) e(StRegP) e(Imm) e(ImmHi)                    \
  /* control transfer (with absolute target address) */ \
  e(Bnz) e(Jmp)                                         \
  /* function call, with absolute target address        \
     or symbol name (external function) */              \
  e(Call) e(CallExt) e(Ret)                             \
  /* debugging */                                       \
  e(Break) e(Error)                                     \
  /* logical operations */                              \
  e(LNot) e(LAnd) e(LOr)                                \
  /* comparisons */                                     \
  e(Eq) e(Ne) e(Gt) e(Lt) e(Ge) e(Le)                   \
  /* arithmetic operations */                           \
  e(Neg) e(Add) e(Sub) e(Mul) e(Div) e(Mod)             \
  /* operand stack operations */                        \
  e(Clear)
// expand macro to comma-separated list
#define VM_EXPAND_LIST(i)         i,
// expand macro to comma-separated string array
#define VM_EXPAND_STR_ARRAY(i)    #i,
// expand macro to label list
#define VM_EXPAND_LABEL_LIST(i)   &&VML_##i,
// define a label of VM threading
#define VM_LABEL(l)               VML_##l:
// goto a label of VM threading
#define VM_GOTO(l)                goto VML_##l

namespace minivm::vm {

// length of VM instruction (in bits)
constexpr std::size_t kVMInstLen = 32;
// length of opcode field (in bits)
constexpr std::size_t kVMInstOpLen = 8;
// length of operand field (in bits)
constexpr std::size_t kVMInstImmLen = kVMInstLen - kVMInstOpLen;

// opcode of VM instructions
enum class InstOp { VM_INSTS(VM_EXPAND_LIST) };

// VM instruction (packed, 'kVMInstLen' bits long)
struct VMInst {
  // opcode
  std::uint32_t op : kVMInstOpLen;
  // symbol reference/immediate/absolute target address
  std::uint32_t opr : kVMInstImmLen;
};

// symbol identifiers
using SymId = std::uint32_t;
// static register identifiers
using RegId = std::uint32_t;
// pc addresses
using VMAddr = std::uint32_t;
// operands
using VMOpr = std::int32_t;

// name of entry point
constexpr const char *kVMEntry = "$entry";
// name of frame area
constexpr const char *kVMFrame = "$frame";
// name of debugger callback
constexpr const char *kVMDebugger = "$debugger";
// name of main function
constexpr const char *kVMMain = "f_main";

// error codes
//
// no error
constexpr std::size_t kVMErrorNoError = 0;
// accessing empty operand stack
constexpr std::size_t kVMErrorEmptyOprStack = 150;
// invalid memory pool address
constexpr std::size_t kVMErrorInvalidMemPoolAddr = 151;
// symbol not found
constexpr std::size_t kVMErrorSymbolNotFound = 152;
// redefining symbol
constexpr std::size_t kVMErrorSymbolRedef = 153;
// invalid register number
constexpr std::size_t kVMErrorInvalidRegNum = 154;
// invalid external function
constexpr std::size_t kVMErrorInvalidExtFunc = 155;
// external function error
constexpr std::size_t kVMErrorExtFuncError = 156;
// invalid PC address
constexpr std::size_t kVMErrorInvalidPCAddr = 157;
// VM irrelevant error
constexpr std::size_t kVMErrorVMIrrelevant = 255;

}  // namespace minivm::vm

#endif  // MINIVM_VM_DEFINE_H_
