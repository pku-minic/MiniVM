#ifndef MINIVM_VM_DEFINE_H_
#define MINIVM_VM_DEFINE_H_

#include <cstdint>

// all supported VM instructions
// for more details, see src/vm/README.md
#define VM_INSTS(e)                                                     \
  /* memory allocation */                                               \
  e(Var) e(Arr)                                                         \
  /* load & store */                                                    \
  e(Ld) e(LdVar) e(LdReg) e(LdAddr) e(St) e(StVar) e(StReg)             \
  e(Imm) e(ImmHi)                                                       \
  /* control transfer (with absolute target address) */                 \
  e(Bnz) e(Jmp)                                                         \
  /* function call                                                      \
     with absolute target address or symbol name (external function) */ \
  e(Call) e(CallExt) e(Ret) e(Param)                                    \
  /* debugging */                                                       \
  e(Break)                                                              \
  /* logical operations */                                              \
  e(LNot) e(LAnd) e(LOr)                                                \
  /* comparisons */                                                     \
  e(Eq) e(Ne) e(Gt) e(Lt) e(Ge) e(Le)                                   \
  /* arithmetic operations */                                           \
  e(Neg) e(Add) e(Sub) e(Mul) e(Div) e(Mod)
// expand macro to comma-separated list
#define VM_EXPAND_LIST(i)         i,
// expand macro to comma-separated string array
#define VM_EXPAND_STR_ARRAY(i)    #i,
// expand macro to label list
#define VM_EXPAND_LABEL_LIST(i)   &&VML_##i,
// define a label of VM threading
#define VM_LABEL(l)               VML_##l:

// opcode of VM instructions
// NOTE: length up to 8 bit, although it's declared as 'std::uint32_t'
enum class InstOp : std::uint32_t { VM_INSTS(VM_EXPAND_LIST) };

// VM instruction (packed, 64-bit long)
struct VMInst {
  // opcode
  InstOp op : 8;
  // symbol reference/immediate/absolute target address
  std::uint32_t opr : 24;
};

// name of entry point
constexpr const char *kVMEntry = "$entry";
// name of frame area
constexpr const char *kVMFrame = "$frame";

#endif  // MINIVM_VM_DEFINE_H_
