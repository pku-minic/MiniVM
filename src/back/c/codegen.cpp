#include "back/c/codegen.h"

#include <cstddef>
#include <cassert>

#include "xstl/embed.h"

using namespace minivm::back::c;
using namespace minivm::vm;

// embbedded C code snippets
static const char *kCCodeTiggerMode = "#define TIGGER_MODE\n";
XSTL_EMBED_STR(kCCodeVM, "back/c/embed/vm.c");
#ifdef LET_CMAKE_KNOW_THERE_IS_A_EMBEDDED_FILE
#include "back/c/embed/vm.c"
#endif

namespace {

// indention
constexpr const char *kIndent = "  ";
// indention x2
constexpr const char *kIndent2 = "    ";
// prefix of label
constexpr const char *kPrefixLabel = "label";
// prefix of parameter array
constexpr const char *kPrefixParams = "params";
// prefix of function
constexpr const char *kPrefixFunc = "VMFunc";
// entry function
constexpr const char *kEntryFunc = "VMEntry";
// label of function end
constexpr const char *kLabelFuncEnd = "label_end";
// stack push operation
constexpr const char *kStackPush = "PushValue";
// stack poke operation
constexpr const char *kStackPoke = "PokeValue";
// stack pop operation
constexpr const char *kStackPop = "PopValue()";
// stack peek operation
constexpr const char *kStackPeek = "PeekValue()";
// stack size
constexpr const char *kStackSize = "StackSize()";
// breakpoint
constexpr const char *kBreakpoint = "Break()";

}  // namespace

std::optional<std::string> CCodeGen::GetSymbol(SymId sym_id, VMAddr pc) {
  // get symbol
  auto sym = cont().sym_pool().FindSymbol(sym_id);
  if (!sym) {
    LogError("symbol not found", pc);
    return {};
  }
  if (sym->front() == 'p') {
    // convert parameter symbol to array access
    std::string ret = kPrefixParams;
    ret += '[';
    ret += sym->substr(1);
    ret += ']';
    return ret;
  }
  else if (sym->front() == '$') {
    // handle symbol starting with '$'
    std::string ret = "builtin_";
    ret += sym->substr(1);
    return ret;
  }
  else {
    return std::string(*sym);
  }
}

std::optional<std::string> CCodeGen::GenerateInst(VMAddr pc,
                                                  const VMInst &inst) {
  std::ostringstream oss;
  // generate pc info
  oss << kIndent << "// pc: " << pc << '\n';
  // generate line info
  auto line = cont().FindLineNum(pc);
  if (line && *line != last_line_) {
    last_line_ = *line;
    auto src = src_reader_.ReadLine(*line);
    if (src) oss << kIndent << "// " << *src << '\n';
    oss << "#line " << *line << " \"" << cont().src_file() << "\"\n";
  }
  // generate label
  if (IsLabel(pc)) oss << kPrefixLabel << pc << ":\n";
  // generate instruction
  auto opcode = static_cast<InstOp>(inst.op);
  switch (opcode) {
    case InstOp::Var: {
      // get symbol of variable
      auto sym = GetSymbol(inst.opr, pc);
      if (!sym) return {};
      // emit C code
      oss << kIndent << "vmopr_t " << *sym << ";\n";
      break;
    }
    case InstOp::Arr: {
      // get symbol of array
      auto sym = GetSymbol(inst.opr, pc);
      if (!sym) return {};
      // emit C code
      oss << kIndent << "vmaddr_t " << *sym << " = pool_sp;\n";
      oss << kIndent << "pool_sp += " << kStackPop << ";\n";
      break;
    }
    case InstOp::Ld: {
      oss << kIndent << kStackPush << "(*(vmopr_t *)(mem_pool + "
          << kStackPop << "));\n";
      break;
    }
    case InstOp::LdVar: {
      // get symbol of variable
      auto sym = GetSymbol(inst.opr, pc);
      if (!sym) return {};
      // emit C code
      oss << kIndent << kStackPush << '(' << *sym << ");\n";
      break;
    }
    case InstOp::LdReg: {
      oss << kIndent << kStackPush << "(regs[" << inst.opr << "]);\n";
      break;
    }
    case InstOp::St: {
      oss << kIndent << "{\n";
      oss << kIndent2 << "vmopr_t *ptr = (vmopr_t *)(mem_pool + "
          << kStackPop << ");\n";
      oss << kIndent2 << "*ptr = " << kStackPop << ";\n";
      oss << kIndent << "}\n";
      break;
    }
    case InstOp::StVar: {
      // get symbol of variable
      auto sym = GetSymbol(inst.opr, pc);
      if (!sym) return {};
      // emit C code
      oss << kIndent << *sym << " = " << kStackPop << ";\n";
      break;
    }
    case InstOp::StVarP: {
      // get symbol of variable
      auto sym = GetSymbol(inst.opr, pc);
      if (!sym) return {};
      // emit C code
      oss << kIndent << *sym << " = " << kStackPeek << ";\n";
      break;
    }
    case InstOp::StReg: {
      oss << kIndent << "regs[" << inst.opr << "] = " << kStackPop << ";\n";
      break;
    }
    case InstOp::StRegP: {
      oss << kIndent << "regs[" << inst.opr << "] = " << kStackPeek
          << ";\n";
      break;
    }
    case InstOp::Imm: {
      oss << kIndent << kStackPush << '(' << inst.opr << ");\n";
      break;
    }
    case InstOp::ImmHi: {
      constexpr auto kMaskLo = (1u << kVMInstImmLen) - 1;
      constexpr auto kMaskHi = (1u << (32 - kVMInstImmLen)) - 1;
      oss << kIndent << kStackPoke << '(' << kStackPeek << " & " << kMaskLo
          << ");\n";
      oss << kIndent << kStackPoke << '(' << kStackPeek << " | "
          << ((inst.opr & kMaskHi) << kVMInstImmLen) << ");\n";
      break;
    }
    case InstOp::Pop: {
      oss << kIndent << kStackPop << ";\n";
      break;
    }
    case InstOp::Bnz: {
      oss << kIndent << "if (" << kStackPop << ") goto " << kPrefixLabel
          << inst.opr << ";\n";
      break;
    }
    case InstOp::Jmp: {
      oss << kIndent << "goto " << kPrefixLabel << inst.opr << ";\n";
      break;
    }
    case InstOp::Call: {
      oss << kIndent << kPrefixFunc << inst.opr << "();\n";
      break;
    }
    case InstOp::CallExt: {
      // get symbol of function
      auto sym = GetSymbol(inst.opr, pc);
      if (!sym) return {};
      // emit C code
      oss << kIndent << *sym << "();\n";
      break;
    }
    case InstOp::Ret: {
      oss << kIndent << "goto " << kLabelFuncEnd << ";\n";
      break;
    }
    case InstOp::Break: {
      oss << kIndent << kBreakpoint << ";\n";
      break;
    }
    default: {
      // arithmetic & logical operations
      if (opcode == InstOp::LNot || opcode == InstOp::Neg) {
        // unary operation
        oss << kIndent << kStackPush << '(';
        oss << "-!"[opcode == InstOp::LNot];
        oss << kStackPop << ");\n";
      }
      else {
        // binary operation
        oss << kIndent << "{\n";
        oss << kIndent2 << "vmopr_t rhs = " << kStackPop << ";\n";
        oss << kIndent2 << kStackPoke << '(' << kStackPeek << ' ';
        switch (opcode) {
          case InstOp::LAnd: oss << "&&"; break;
          case InstOp::LOr: oss << "||"; break;
          case InstOp::Eq: oss << "=="; break;
          case InstOp::Ne: oss << "!="; break;
          case InstOp::Gt: oss << ">"; break;
          case InstOp::Lt: oss << "<"; break;
          case InstOp::Ge: oss << ">="; break;
          case InstOp::Le: oss << "<="; break;
          case InstOp::Add: oss << '+'; break;
          case InstOp::Sub: oss << '-'; break;
          case InstOp::Mul: oss << '*'; break;
          case InstOp::Div: oss << '/'; break;
          case InstOp::Mod: oss << '%'; break;
          default: assert(false);
        }
        oss << " rhs);\n";
        oss << kIndent << "}\n";
      }
    }
  }
  return oss.str();
}

void CCodeGen::Reset() {
  global_.str("");
  global_.clear();
  code_.str("");
  code_.clear();
  last_line_ = 0;
  // add code snippets
  if (tigger_mode_) global_ << kCCodeTiggerMode;
  global_ << kCCodeVM << '\n';
}

void CCodeGen::GenerateOnFunc(VMAddr pc, const FuncBody &func) {
  // generate function body
  auto cur_pc = pc;
  std::ostringstream body;
  for (const auto &inst : func) {
    auto code = GenerateInst(cur_pc, inst);
    if (!code) return;
    body << *code;
    ++cur_pc;
  }
  // generate function
  code_ << "void " << kPrefixFunc << pc << "() {\n";
  code_ << kIndent << "vmaddr_t pool_bp = pool_sp;\n";
  if (!tigger_mode_) {
    code_ << kIndent << "vmopr_t *" << kPrefixParams
          << " = (vmopr_t *)(mem_pool + pool_sp);\n";
    code_ << kIndent << "pool_sp += " << kStackSize << " * 4;\n";
    code_ << kIndent << "while (" << kStackSize << ") {\n";
    code_ << kIndent2 << "size_t i = " << kStackSize << " - 1;\n";
    code_ << kIndent2 << kPrefixParams << "[i] = " << kStackPop << ";\n";
    code_ << kIndent << "}\n\n";
  }
  else {
    code_ << '\n';
  }
  code_ << body.str() << '\n';
  code_ << kLabelFuncEnd << ":\n";
  code_ << kIndent << "pool_sp = pool_bp;\n";
  code_ << "}\n\n";
}

void CCodeGen::GenerateOnEntry(VMAddr pc, const FuncBody &func) {
  // generate function body
  auto cur_pc = pc;
  std::ostringstream body;
  for (const auto &inst : func) {
    // generate instructions
    switch (static_cast<InstOp>(inst.op)) {
      // global variables
      case InstOp::Var: {
        // get symbol of variable
        auto sym = GetSymbol(inst.opr, pc);
        if (!sym) return;
        // emit C code
        global_ << "vmopr_t " << *sym << ";\n";
        break;
      }
      // global arrays
      case InstOp::Arr: {
        // get symbol of array
        auto sym = GetSymbol(inst.opr, pc);
        if (!sym) return;
        // emit C code
        global_ << "vmaddr_t " << *sym << ";\n";
        body << kIndent << *sym << " = pool_sp;\n";
        body << kIndent << "pool_sp += " << kStackPop << ";\n";
        break;
      }
      // other instructions
      default: {
        auto code = GenerateInst(cur_pc, inst);
        if (!code) return;
        body << *code;
        break;
      }
    }
    // update current pc
    ++cur_pc;
  }
  // generate functions
  code_ << "void " << kEntryFunc << "() {\n";
  code_ << body.str() << '\n';
  code_ << kLabelFuncEnd << ":\n";
  code_ << kIndent << "(void)0;\n";
  code_ << "}\n\n";
}

void CCodeGen::Dump(std::ostream &os) const {
  os << global_.str() << '\n' << code_.str();
}
