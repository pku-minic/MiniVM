#include "debugger/minidbg/minidbg.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include <cctype>

#include "front/token.h"

#define CMD_HANDLER(func) [this](std::istream &is) { return func(is); }

namespace {

// name of static registers
const char *kRegNames[] = {TOKEN_REGISTERS(TOKEN_EXPAND_SECOND)};

// item of 'info' command
enum class InfoItem {
  Stack, Env, Reg, Break, Watch,
};

// string -> InfoItem
const std::unordered_map<std::string_view, InfoItem> kInfoItems = {
    {"stack", InfoItem::Stack}, {"s", InfoItem::Stack},
    {"env", InfoItem::Env},     {"e", InfoItem::Env},
    {"reg", InfoItem::Reg},     {"r", InfoItem::Reg},
    {"break", InfoItem::Break}, {"b", InfoItem::Break},
    {"watch", InfoItem::Watch}, {"w", InfoItem::Watch},
};

}  // namespace

void MiniDebugger::InitDebuggerCommands() {
  RegisterCommand("break", "b", CMD_HANDLER(CreateBreak), "[POS]",
                  "set breakpoint at POS",
                  "Set a breakpoint at specific address (PC), "
                  "POS defaults to current PC.");
  RegisterCommand("watch", "w", CMD_HANDLER(CreateWatch), "EXPR",
                  "set watchpoint at EXPR",
                  "Set a watchpoint for a specific expression, "
                  "pause when EXPR changes.\n"
                  "  Setting watchpoints may cause MiniVM to run slowly.");
  RegisterCommand("delete", "d", CMD_HANDLER(DeletePoint), "[N]",
                  "delete breakpoint/watchpoint",
                  "Delete breakpoint/watchpoint N, delete all "
                  "breakpoints and watchpoints by default.");
  RegisterCommand("continue", "c", CMD_HANDLER(Continue), "",
                  "continue running", "Continue running current program.");
  RegisterCommand("next", "n", CMD_HANDLER(NextLine), "",
                  "stepping over calls (source level)",
                  "Source level single step, stepping over calls.");
  RegisterCommand("nexti", "ni", CMD_HANDLER(NextInst), "[N]",
                  "stepping over calls (instruction level)",
                  "Perform N instruction level single steps, "
                  "stepping over calls. N defaults to 1.");
  RegisterCommand("step", "s", CMD_HANDLER(StepLine), "",
                  "stepping into calls (source level)",
                  "Source level single step, stepping into calls.");
  RegisterCommand("stepi", "si", CMD_HANDLER(StepInst), "[N]",
                  "stepping into calls (instruction level)",
                  "Perform N instruction level single steps, "
                  "stepping into calls. N defaults to 1.");
  RegisterCommand("print", "p", CMD_HANDLER(PrintExpr), "[EXPR]",
                  "show value of EXPR",
                  "Show value of EXPR, or just show the last value.");
  RegisterCommand("x", "", CMD_HANDLER(ExamineMem), "N EXPR",
                  "examine memory at EXPR",
                  "Examine N units memory at address EXPR, "
                  "4 bytes per unit.");
  RegisterCommand("info", "", CMD_HANDLER(PrintInfo), "ITEM",
                  "show information of ITEM",
                  "Show information of ITEM.\n\n"
                  "ITEM:\n"
                  "  stack/s  --- operand stack\n"
                  "  env/e    --- environment stack\n"
                  "  reg/r    --- static registers\n"
                  "  break/b  --- breakpoints\n"
                  "  watch/w  --- watchpoints");
  RegisterCommand("layout", "", CMD_HANDLER(SetLayout), "FMT",
                  "set layout of disassembler",
                  "Set layout of disassembler, FMT can be 'src' or 'asm'");
  RegisterCommand("disasm", "da", CMD_HANDLER(DisasmMem), "[N POS]",
                  "set layout of automatic disassemble",
                  "Disassemble N loc/instructions at POS, "
                  "disassemble 10 loc near current PC by default.");
}

void MiniDebugger::RegisterDebuggerCallback() {
  auto ret = vm_.RegisterFunction(
      kVMDebugger, [this](VM &vm) { return DebuggerCallback(); });
  assert(ret);
  static_cast<void>(ret);
}

bool MiniDebugger::DebuggerCallback() {
  // enter command line interface
  EnterCLI();
  // disable trap mode and continue
  vm_.cont().ToggleTrapMode(false);
  return true;
}

void MiniDebugger::LogError(std::string_view message) {
  std::cout << "ERROR (debugger): " << message << std::endl;
}

std::optional<VMAddr> MiniDebugger::ReadPosition(std::istream &is) {
  // read position string
  std::string pos;
  is >> pos;
  if (pos[0] == ':') {
    // line number
    char *end_pos;
    auto line_num = std::strtol(pos.c_str() + 1, &end_pos, 10);
    if (*end_pos) {
      LogError("invalid line number");
      return {};
    }
    // get PC address by line number
    auto addr = vm_.cont().FindPC(line_num);
    if (!addr) LogError("line number out of range");
    return addr;
  }
  else if (std::isdigit(pos[0])) {
    // PC address
    char *end_pos;
    VMAddr addr = std::strtol(pos.c_str(), &end_pos, 10);
    if (*end_pos) {
      LogError("invalid PC address");
      return {};
    }
    return addr;
  }
  else {
    // function or label
    auto addr = vm_.cont().FindPC(pos);
    if (!addr) LogError("function/label not found");
    return addr;
  }
}

std::optional<VMOpr> MiniDebugger::ReadExpression(std::istream &is) {
  return ReadExpression(is, true);
}

std::optional<VMOpr> MiniDebugger::ReadExpression(std::istream &is,
                                                   bool record) {
  // read expression from input stream
  std::string expr;
  if (!std::getline(is, expr)) {
    LogError("invalid 'EXPR'");
    return {};
  }
  // evaluate expression
  return eval_.Eval(expr, record);
}

bool MiniDebugger::DeleteBreak(std::uint32_t id) {
  // try to find breakpoint info
  auto it = breaks_.find(id);
  if (it == breaks_.end()) return false;
  const auto &info = it->second;
  // delete breakpoint
  vm_.cont().ToggleBreakpoint(info.addr, false);
  pc_bp_.erase(info.addr);
  breaks_.erase(it);
  return true;
}

bool MiniDebugger::DeleteWatch(std::uint32_t id) {
  // try to find watchpoint info
  auto it = watches_.find(id);
  if (it == watches_.end()) return false;
  const auto &info = it->second;
  // delete watchpoint
  eval_.RemoveRecord(info.record_id);
  watches_.erase(it);
  return true;
}

void MiniDebugger::PrintStackInfo() {
  const auto &oprs = vm_.oprs();
  std::cout << "operand stack size: " << oprs.size() << std::endl;
  if (!oprs.empty()) {
    std::cout << "top of stack: " << oprs.top() << std::endl;
  }
}

void MiniDebugger::PrintEnv(const VM::EnvPtr &env) {
  if (env->empty()) {
    std::cout << "  <empty>" << std::endl;
  }
  else {
    for (const auto &[sym_id, val] : *env) {
      auto sym = vm_.sym_pool().FindSymbol(sym_id);
      assert(sym);
      std::cout << "  " << *sym << " = " << val << std::endl;
    }
  }
}

void MiniDebugger::PrintEnvInfo() {
  const auto &[env, addr] = vm_.env_addr_pair();
  std::cout << "return address: " << addr << std::endl;
  std::cout << "current environment:" << std::endl;
  PrintEnv(env);
  std::cout << "global environment:" << std::endl;
  PrintEnv(vm_.global_env());
}

void MiniDebugger::PrintRegInfo() {
  // check if in Tigger mode
  const auto &[env, _] = vm_.env_addr_pair();
  auto id = vm_.sym_pool().FindId(kVMFrame);
  assert(id);
  if (env->find(*id) != env->end()) {
    std::cout << "WARNING: MiniVM may not currently run in Tigger mode, "
                 "static registers should not be used."
              << std::endl << std::endl;
  }
  // print PC
  std::cout << "current PC address: " << vm_.pc() << std::endl;
  // print value of static registers
  std::cout << "static registers:" << std::endl << "  ";
  int count = 0;
  for (RegId i = 0; i < TOKEN_COUNT(TOKEN_REGISTERS); ++i) {
    // print value of register
    auto val = vm_.regs(i);
    std::cout << std::setw(4) << std::setfill(' ') << std::left
              << kRegNames[i] << std::hex << std::setw(8)
              << std::setfill('0') << std::right << val << "   ";
    // print new line
    if (count++ == 4) {
      count = 0;
      std::cout << std::endl << "  ";
    }
  }
  if (count) std::cout << std::endl;
}

void MiniDebugger::PrintBreakInfo() {
  if (breaks_.empty()) {
    std::cout << "no breakpoints currently set" << std::endl;
  }
  else {
    std::cout << "number of breakpoints: " << breaks_.size() << std::endl;
    for (const auto &[id, info] : breaks_) {
      std::cout << "  breakpoint #" << id << ": pc = 0x" << std::hex
                << std::setw(8) << std::setfill('0') << info.addr
                << std::dec << ", hit_count = " << info.hit_count
                << std::endl;
    }
  }
}

void MiniDebugger::PrintWatchInfo() {
  if (watches_.empty()) {
    std::cout << "no watchpoints currently set" << std::endl;
  }
  else {
    std::cout << "number of watchpoints: " << watches_.size() << std::endl;
    for (const auto &[id, info] : watches_) {
      std::cout << "  watchpoint #" << id << ": $" << info.record_id
                << " = '";
      eval_.PrintExpr(std::cout, info.record_id);
      std::cout << "', value = " << info.last_val
                << ", hit_count = " << info.hit_count << std::endl;
    }
  }
}

void MiniDebugger::ShowDisasm() {
  auto pc = vm_.pc();
  std::size_t n = 10;
  if (layout_fmt_ == LayoutFormat::Asm) {
    if (pc >= 2) pc -= 2;
  }
  else {
    assert(layout_fmt_ == LayoutFormat::Source);
    // TODO
  }
  ShowDisasm(pc, n);
}

void MiniDebugger::ShowDisasm(VMAddr pc, std::size_t n) {
  // TODO
}

bool MiniDebugger::CreateBreak(std::istream &is) {
  // get address of breakpoint
  auto addr = is.eof() ? vm_.pc() : ReadPosition(is);
  // check for duplicates
  if (pc_bp_.find(*addr) != pc_bp_.end()) {
    LogError("there is already a breakpoint at the specific POS");
    return false;
  }
  // toggle breakpoint
  vm_.cont().ToggleBreakpoint(*addr, true);
  // store breakpoint info
  auto ret = breaks_.insert({next_id_++, {*addr, 0}});
  assert(ret.second);
  pc_bp_.insert({*addr, &ret.first->second});
  return false;
}

bool MiniDebugger::CreateWatch(std::istream &is) {
  // get record id of expression
  auto id = eval_.next_id();
  // evaluate expression and record
  auto val = ReadExpression(is);
  if (!val) return false;
  // store watchpoint info
  auto succ = watches_.insert({next_id_++, {id, *val, 0}}).second;
  assert(succ);
  static_cast<void>(succ);
}

bool MiniDebugger::DeletePoint(std::istream &is) {
  if (is.eof()) {
    // show confirm message
    std::cout << "are you sure to delete all "
                 "breakpoints & watchpoints? [y/n] ";
    std::string line;
    if (!std::getline(std::cin, line) || line.size() != 1 ||
        std::tolower(line[0]) != 'y') {
      return false;
    }
    // delete all breakpoints
    auto breaks = breaks_;
    for (const auto &i : breaks) DeleteBreak(i.first);
    // delete all watchpoints
    auto watches = watches_;
    for (const auto &i : watches) DeleteWatch(i.first);
  }
  else {
    // get id from input
    std::uint32_t n;
    is >> n;
    if (!is) {
      LogError("invalid breakpoint/watchpoint id");
      return false;
    }
    // delete point by id
    if (!DeleteBreak(n) && !DeleteWatch(n)) {
      LogError("breakpoint/watchpoint not found");
    }
  }
  return false;
}

bool MiniDebugger::Continue(std::istream &is) {
  // just quit CLI and continue
  return true;
}

bool MiniDebugger::NextLine(std::istream &is) {
  // TODO
  return true;
}

bool MiniDebugger::NextInst(std::istream &is) {
  // TODO
  return true;
}

bool MiniDebugger::StepLine(std::istream &is) {
  // TODO
  return true;
}

bool MiniDebugger::StepInst(std::istream &is) {
  // TODO
  return true;
}

bool MiniDebugger::PrintExpr(std::istream &is) {
  std::optional<VMOpr> value;
  std::uint32_t id;
  // get expression
  if (is.eof()) {
    // show last value
    id = eval_.next_id();
    while (!value) {
      if (!id) {
        LogError("there is no last value available");
        return false;
      }
      value = eval_.Eval(--id);
    }
  }
  else {
    // evaluate expression
    id = eval_.next_id();
    if (!(value = ReadExpression(is))) return false;
  }
  // print result
  std::cout << "$" << id << " = " << *value << std::endl;
  return false;
}

bool MiniDebugger::ExamineMem(std::istream &is) {
  // get number N
  std::uint32_t n;
  is >> n;
  if (!is || !n) {
    LogError("invalid number N");
    return false;
  }
  // get expression
  auto val = ReadExpression(is, false);
  if (!val) return false;
  // print memory units
  auto addr = *val;
  while (n--) {
    // print address
    std::cout << std::hex << std::setfill('0') << std::setw(8) << addr;
    // get pointer of current unit
    auto ptr = reinterpret_cast<char *>(vm_.mem_pool()->GetAddress(addr));
    addr += 4;
    // print contents
    std::cout << ": " << std::setw(2) << std::setfill('0')
              << static_cast<int>(*ptr++) << ' ';
    std::cout << std::setw(2) << std::setfill('0')
              << static_cast<int>(*ptr++) << ' ';
    std::cout << std::setw(2) << std::setfill('0')
              << static_cast<int>(*ptr++) << ' ';
    std::cout << std::setw(2) << std::setfill('0')
              << static_cast<int>(*ptr++);
    std::cout << std::dec << std::endl;
  }
  return false;
}

bool MiniDebugger::PrintInfo(std::istream &is) {
  // get items
  std::string item_str;
  is >> item_str;
  auto it = kInfoItems.find(item_str);
  if (it == kInfoItems.end()) {
    LogError("invalid 'ITEM'");
    return false;
  }
  // print information
  switch (it->second) {
    case InfoItem::Stack: PrintStackInfo(); break;
    case InfoItem::Env: PrintEnvInfo(); break;
    case InfoItem::Reg: PrintRegInfo(); break;
    case InfoItem::Break: PrintBreakInfo(); break;
    case InfoItem::Watch: PrintWatchInfo(); break;
    default: assert(false);
  }
  return false;
}

bool MiniDebugger::SetLayout(std::istream &is) {
  // get format
  std::string fmt;
  is >> fmt;
  // parse format
  if (fmt == "src") {
    layout_fmt_ = LayoutFormat::Source;
  }
  else if (fmt == "asm") {
    layout_fmt_ = LayoutFormat::Asm;
  }
  else {
    LogError("invalid layout format");
  }
  return false;
}

bool MiniDebugger::DisasmMem(std::istream &is) {
  // no parameters
  if (is.eof()) {
    ShowDisasm();
  }
  else {
    // get parameters
    std::size_t n;
    is >> n;
    if (!is || !n) {
      LogError("invalid count 'N'");
      return false;
    }
    auto pc = ReadExpression(is, false);
    if (!pc) {
      LogError("invalid 'EXPR'");
      return false;
    }
    // show disassembly
    ShowDisasm(*pc, n);
  }
  return false;
}
