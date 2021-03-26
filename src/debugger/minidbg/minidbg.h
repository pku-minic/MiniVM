#ifndef MINIVM_DEBUGGER_MINIDBG_MINIDBG_H_
#define MINIVM_DEBUGGER_MINIDBG_MINIDBG_H_

#include <string_view>
#include <optional>
#include <unordered_map>
#include <cstdint>

#include "debugger/debugger.h"
#include "vm/vm.h"
#include "debugger/minidbg/minieval.h"

// debugger for MiniVM
class MiniDebugger : public DebuggerBase {
 public:
  MiniDebugger(VM &vm)
      : DebuggerBase("minidbg> "), vm_(vm), eval_(vm), next_id_(0) {
    InitDebuggerCommands();
    RegisterDebuggerCallback();
  }

 private:
  // breakpoint information
  struct BreakInfo {
    // PC address of breakpoint
    VMAddr addr;
    // hit count
    std::uint32_t hit_count;
  };

  // watchpoint information
  struct WatchInfo {
    // expression record id (in 'ExprEvaluator')
    std::uint32_t record_id;
    // last value
    std::uint32_t last_val;
    // hit count
    std::uint32_t hit_count;
  };

  // item of 'info' command
  enum class InfoItem {
    Stack, Env, Reg, Break, Watch,
  };

  // layout type of auto-disasm
  enum class DisasmLayoutType {
    Source, Asm,
  };

  // initialize debugger commands
  void InitDebuggerCommands();
  // register debugger callback for current instance
  void RegisterDebuggerCallback();
  // debugger callback
  bool DebuggerCallback();
  // log error message to stdout
  void LogError(std::string_view message);

  // read POS from input stream
  std::optional<VMAddr> ReadPosition(std::istream &is);
  // read EXPR from input stream
  std::optional<VMAddr> ReadExpression(std::istream &is);
  // delete the specific breakpoint
  // returns false if breakpoint not found
  bool DeleteBreak(std::uint32_t id);
  // delete the specific watchpoint
  // returns false if watchpoint not found
  bool DeleteWatch(std::uint32_t id);

  // create a new breakpoint ('break [POS]')
  bool CreateBreak(std::istream &is);
  // create a new watchpoint ('watch EXPR')
  bool CreateWatch(std::istream &is);
  // delete breakpoint/watchpoint ('delete [N]')
  bool DeletePoint(std::istream &is);
  // continue running ('continue')
  bool Continue(std::istream &is);
  // source level single step, stepping over calls ('next')
  bool NextLine(std::istream &is);
  // instruction level single step, stepping over calls ('nexti [N]')
  bool NextInst(std::istream &is);
  // source level single step, stepping into calls ('step')
  bool StepLine(std::istream &is);
  // instruction level single step, stepping into calls ('stepi [N]')
  bool StepInst(std::istream &is);
  // print value of expression ('print [EXPR]')
  bool PrintExpr(std::istream &is);
  // examine memory ('x N EXPR')
  bool ExamineMem(std::istream &is);
  // print information ('info ITEM')
  bool PrintInfo(std::istream &is);
  // set layout of automatic disassemble ('layout TYPE')
  bool SetLayout(std::istream &is);
  // disassemble memory ('disasm [N POS]')
  bool DisasmMem(std::istream &is);

  // current MiniVM instance
  VM &vm_;
  // expression evaluator
  MiniEvaluator eval_;
  // next breakpoint/watchpoint id
  std::uint32_t next_id_;
  // all breakpoints
  std::unordered_map<std::uint32_t, BreakInfo> breaks_;
  // hashmap of PC address to breakpoint info reference
  std::unordered_map<VMAddr, BreakInfo *> pc_bp_;
  // all watchpoints
  std::unordered_map<std::uint32_t, WatchInfo> watches_;
};

#endif  // MINIVM_DEBUGGER_MINIDBG_MINIDBG_H_
