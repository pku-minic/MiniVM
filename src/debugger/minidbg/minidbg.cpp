#include "debugger/minidbg/minidbg.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cctype>

#include "front/token.h"
#include "xstl/style.h"
#include "xstl/segfault.h"

// create a new anonymous command handler
#define CMD_HANDLER(func) [this](std::istream &is) { return func(is); }

namespace {

// instruction information (for disassembler)
struct InstInfo {
  bool        is_breakpoint;
  VMAddr      addr;
  std::string disasm;
};

// line information (for disassembler)
struct LineInfo {
  bool              is_breakpoint;
  std::uint32_t     addr;   // line number
  std::string_view  disasm;
};

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

// print instructions to stdout
template <typename Info, typename Addr>
void PrintInst(const std::vector<Info> &info, Addr cur_addr) {
  using namespace xstl;
  assert(!info.empty());
  // check if there is a breakpoint
  bool print_bp = std::find_if(info.begin(), info.end(), [](const Info &i) {
                    return i.is_breakpoint;
                  }) != info.end();
  // get maximum address
  auto max_addr = std::max_element(info.begin(), info.end(),
                                   [](const Info &l, const Info &r) {
                                     return l.addr < r.addr;
                                   })->addr;
  auto addr_padd =
      static_cast<int>(std::ceil(std::log10(max_addr + 1))) + 2;
  // print instruction info
  std::cout << std::endl;
  for (const auto &[is_break, addr, disasm] : info) {
    // print breakpoint info
    if (print_bp) {
      if (is_break) {
        std::cout << style("D") << " B> ";
      }
      else {
        std::cout << "    ";
      }
    }
    // print current address
    if (addr == cur_addr) {
      std::cout << style("I") << std::setw(addr_padd) << std::setfill(' ')
                << std::right << addr << style("R") << ":  ";
    }
    else {
      std::cout << std::setw(addr_padd) << std::setfill(' ') << std::right
                << addr << ":  ";
    }
    // print disassembly
    std::cout << style("B") << disasm;
    std::cout << std::endl;
  }
}

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
                  "Set layout of disassembler, FMT can be 'src' or 'asm'.");
  RegisterCommand("disasm", "da", CMD_HANDLER(DisasmMem), "[N POS]",
                  "Show source code, or disassemble VM instructions",
                  "Disassemble N loc/instructions at POS, "
                  "disassemble 10 loc near current PC by default.");
}

void MiniDebugger::RegisterDebuggerCallback() {
  auto ret = vm_.RegisterFunction(
      kVMDebugger, [this](VM &vm) { return DebuggerCallback(); });
  assert(ret);
  static_cast<void>(ret);
}

void MiniDebugger::InitSigIntHandler() {
  set_sigint_handler([this] { vm_.cont().ToggleTrapMode(true); });
}

bool MiniDebugger::DebuggerCallback() {
  if (!vm_.cont().FindLineNum(vm_.pc())) {
    // skip when line number unavailable
    StepLineHandler({});
  }
  else {
    // check breakpoint status
    CheckBreakpoints();
    // show disassembly
    ShowDisasm();
    // enter command line interface
    EnterCLI();
  }
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

void MiniDebugger::CheckBreakpoints() {
  auto cur_pc = vm_.pc();
  auto &cont = vm_.cont();
  // check if breakpoint is hit
  if (auto it = pc_bp_.find(cur_pc); it != pc_bp_.end()) {
    // toggle breakpoint
    cont.ToggleBreakpoint(cur_pc, false);
    cont.AddStepCounter(1, [this, cur_pc](VMInstContainer &cont) {
      if (pc_bp_.count(cur_pc)) cont.ToggleBreakpoint(cur_pc, true);
    });
    // update hit counter
    ++it->second->hit_count;
    // print PC address
    std::cout << "breakpoint hit, pc = " << cur_pc;
    // print line number
    auto line = cont.FindLineNum(cur_pc);
    assert(line);
    std::cout << ", at line " << *line << std::endl;
  }
}

void MiniDebugger::CheckWatchpoints() {
  bool break_flag = false;
  for (auto &&it : watches_) {
    // get watchpoint info
    auto &info = it.second;
    // evaluate new value
    auto val = eval_.Eval(info.record_id);
    if (val && *val != info.last_val) {
      break_flag = true;
      // record change
      std::cout << "watchpoint #" << it.first << " hit ($"
                << info.record_id << ")" << std::endl;
      std::cout << "  old value: " << info.last_val << std::endl;
      std::cout << "  new value: " << *val << std::endl;
      // update watchpoint info
      info.last_val = *val;
      ++info.hit_count;
    }
  }
  // break if there are any watchpoints changed
  if (break_flag) vm_.cont().ToggleTrapMode(true);
  // update step counter
  if (!watches_.empty()) {
    vm_.cont().AddStepCounter(0, [this](VMInstContainer &cont) {
      CheckWatchpoints();
    });
  }
}

void MiniDebugger::NextLineHandler(std::uint32_t line, std::size_t depth) {
  auto cur_line = vm_.cont().FindLineNum(vm_.pc());
  if (!depth && cur_line != line) {
    // the line after the function call has been fetched
    vm_.cont().ToggleTrapMode(true);
  }
  else {
    // still in function call
    auto cur_inst = vm_.cont().insts() + vm_.pc();
    // update depth
    auto new_depth = depth;
    switch (static_cast<InstOp>(cur_inst->op)) {
      case InstOp::Call: ++new_depth; break;
      case InstOp::Ret: --new_depth; break;
      default:;
    }
    // update step counter
    vm_.cont().AddStepCounter(
        0, [this, line, new_depth](VMInstContainer &cont) {
          NextLineHandler(line, new_depth);
        });
  }
}

void MiniDebugger::NextInstHandler(std::size_t n) {
  if (!n) {
    // stop execution
    vm_.cont().ToggleTrapMode(true);
  }
  else {
    // step over the current instruction
    auto cur_inst = vm_.cont().insts() + vm_.pc();
    if (static_cast<InstOp>(cur_inst->op) == InstOp::Call) {
      // wait until instruction at 'pc + 1' has been fetched
      NextInstHandler(n - 1, vm_.pc() + 1, 0);
    }
    else {
      // just step
      vm_.cont().AddStepCounter(0, [this, n](VMInstContainer &cont) {
        NextInstHandler(n - 1);
      });
    }
  }
}

void MiniDebugger::NextInstHandler(std::size_t n, std::size_t next_pc,
                                   std::size_t depth) {
  if (vm_.pc() == next_pc && !depth) {
    // the instruction after the 'Call' instruction has been fetched
    if (!n) {
      // stop execution
      vm_.cont().ToggleTrapMode(true);
    }
    else {
      // go for next step
      vm_.cont().AddStepCounter(0, [this, n](VMInstContainer &cont) {
        NextInstHandler(n - 1);
      });
    }
  }
  else {
    // still in function call
    auto cur_inst = vm_.cont().insts() + vm_.pc();
    // update depth
    auto new_depth = depth;
    switch (static_cast<InstOp>(cur_inst->op)) {
      case InstOp::Call: ++new_depth; break;
      case InstOp::Ret: --new_depth; break;
      default:;
    }
    // update step counter
    vm_.cont().AddStepCounter(
        0, [this, n, next_pc, new_depth](VMInstContainer &cont) {
          NextInstHandler(n, next_pc, new_depth);
        });
  }
}

void MiniDebugger::StepLineHandler(std::optional<std::uint32_t> line) {
  auto cur_line = vm_.cont().FindLineNum(vm_.pc());
  if (cur_line == line) {
    // line position not changed, continue stepping
    vm_.cont().AddStepCounter(0, [this, line](VMInstContainer &cont) {
      StepLineHandler(line);
    });
  }
  else {
    // stop execution
    vm_.cont().ToggleTrapMode(true);
  }
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
  if (!id || env->find(*id) != env->end()) {
    return LogError("MiniVM may not currently run in Tigger mode, "
                    "static registers should not be used.");
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
      std::cout << "  breakpoint #" << id << ": pc = " << info.addr
                << ", hit_count = " << info.hit_count << std::endl;
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
  if (layout_fmt_ == LayoutFormat::Asm) {
    pc = pc >= 2 ? pc - 2 : 0;
  }
  else {
    assert(layout_fmt_ == LayoutFormat::Source);
    // get line number of current PC
    auto line = vm_.cont().FindLineNum(pc);
    assert(line);
    // get start PC address
    auto line_start = *line >= 2 ? *line - 2 : 0;
    auto pc_start = vm_.cont().FindPC(line_start);
    if (pc_start) pc = *pc_start;
  }
  ShowDisasm(pc, 10);
}

void MiniDebugger::ShowDisasm(VMAddr pc, std::size_t n) {
  if (layout_fmt_ == LayoutFormat::Asm) {
    std::vector<InstInfo> info;
    // add instructions to 'info'
    for (std::size_t i = 0; i < n; ++i) {
      VMAddr cur_pc = pc + i;
      std::ostringstream oss;
      if (!vm_.cont().Dump(oss, cur_pc)) break;
      info.push_back({!!pc_bp_.count(cur_pc), cur_pc, oss.str()});
    }
    // print instruction info list
    PrintInst(info, vm_.pc());
  }
  else {
    assert(layout_fmt_ == LayoutFormat::Source);
    std::vector<LineInfo> info;
    // get start line number of the pc & current line number
    auto line_no = vm_.cont().FindLineNum(pc);
    auto cur_line_no = vm_.cont().FindLineNum(vm_.pc());
    assert(line_no && cur_line_no);
    // add lines to 'info'
    for (std::size_t i = 0; i < n; ++i) {
      // read the current line
      std::uint32_t cur_no = *line_no + i;
      auto line = src_reader_.ReadLine(cur_no);
      if (line.empty()) break;
      // check if there is a breakpoint
      auto cur_pc = vm_.cont().FindPC(cur_no);
      auto is_break = cur_pc && pc_bp_.count(*cur_pc);
      info.push_back({is_break, cur_no, line});
    }
    // print line info list
    PrintInst(info, *cur_line_no);
  }
}

bool MiniDebugger::CreateBreak(std::istream &is) {
  // get address of breakpoint
  auto addr = is.eof() ? vm_.pc() : ReadPosition(is);
  // check for duplicates
  if (pc_bp_.count(*addr)) {
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
  if (!val) {
    LogError("invalid expression");
    return false;
  }
  // store watchpoint info
  auto succ = watches_.insert({next_id_++, {id, *val, 0}}).second;
  assert(succ);
  static_cast<void>(succ);
  // create step counter for watchpoint updating
  if (watches_.size() == 1) CheckWatchpoints();
  return false;
}

bool MiniDebugger::DeletePoint(std::istream &is) {
  if (is.eof()) {
    // show confirm message
    std::cout << "are you sure to delete all "
                 "breakpoints & watchpoints? [y/n] ";
    std::string line;
    std::cin >> line;
    if (!std::cin || line.size() != 1 || std::tolower(line[0]) != 'y') {
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
  auto line = vm_.cont().FindLineNum(vm_.pc());
  auto cur_inst = vm_.cont().insts() + vm_.pc();
  if (static_cast<InstOp>(cur_inst->op) == InstOp::Call) {
    // step until reaching the next line
    assert(line);
    auto cur_line = *line;
    vm_.cont().AddStepCounter(0, [this, cur_line](VMInstContainer &cont) {
      NextLineHandler(cur_line, 0);
    });
  }
  else {
    // just same as step into
    StepLineHandler(line);
  }
  return true;
}

bool MiniDebugger::NextInst(std::istream &is) {
  std::size_t n = 1;
  if (!is.eof()) {
    // n steps
    is >> n;
    if (!is || !n) {
      LogError("invalid step count");
      return false;
    }
  }
  // add step counter
  vm_.cont().AddStepCounter(0, [this, n](VMInstContainer &cont) {
    NextInstHandler(n);
  });
  return true;
}

bool MiniDebugger::StepLine(std::istream &is) {
  auto line = vm_.cont().FindLineNum(vm_.pc());
  StepLineHandler(line);
  return true;
}

bool MiniDebugger::StepInst(std::istream &is) {
  std::size_t n = 1;
  if (!is.eof()) {
    // n steps
    is >> n;
    if (!is || !n) {
      LogError("invalid step count");
      return false;
    }
  }
  // enable step mode
  vm_.cont().AddStepCounter(n);
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
    if (!(value = ReadExpression(is))) {
      LogError("invalid expression");
      return false;
    }
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
  TRY_SEGFAULT {
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
  }
  CATCH_SEGFAULT {
    // segmentation fault
    std::cout << std::endl;
    LogError("segmentation fault");
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
    auto pos = ReadPosition(is);
    if (!pos) {
      LogError("invalid 'POS'");
      return false;
    }
    // show disassembly
    ShowDisasm(*pos, n);
  }
  return false;
}
