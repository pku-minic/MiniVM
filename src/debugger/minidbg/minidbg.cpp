#include "debugger/minidbg/minidbg.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include <cctype>

#define CMD_HANDLER(func) [this](std::istream &is) { return func(is); }

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
  RegisterCommand("layout", "", CMD_HANDLER(SetLayout), "TYPE",
                  "set layout of automatic disassemble",
                  "Set layout of automatic disassemble, "
                  "TYPE can be 'src' or 'asm'");
  RegisterCommand("disasm", "da", CMD_HANDLER(DisasmMem), "[N POS]",
                  "set layout of automatic disassemble",
                  "Disassemble N units memory at POS, "
                  "disassemble 10 loc near current line by default.");
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

std::optional<VMAddr> MiniDebugger::ReadExpression(std::istream &is) {
  // read expression from input stream
  std::string expr;
  if (!std::getline(is, expr)) {
    LogError("invalid 'EXPR'");
    return {};
  }
  // evaluate expression
  return eval_.Eval(expr);
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
  //
}

bool MiniDebugger::NextInst(std::istream &is) {
  //
}

bool MiniDebugger::StepLine(std::istream &is) {
  //
}

bool MiniDebugger::StepInst(std::istream &is) {
  //
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
}

bool MiniDebugger::ExamineMem(std::istream &is) {
  //
}

bool MiniDebugger::PrintInfo(std::istream &is) {
  //
}

bool MiniDebugger::SetLayout(std::istream &is) {
  //
}

bool MiniDebugger::DisasmMem(std::istream &is) {
  //
}
