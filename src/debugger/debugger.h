#ifndef MINIVM_DEBUGGER_DEBUGGER_H_
#define MINIVM_DEBUGGER_DEBUGGER_H_

#include <string_view>
#include <functional>
#include <istream>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <csignal>

// command line interface of debugger
class DebuggerBase {
 public:
  DebuggerBase(std::string_view prompt) : prompt_(prompt) {
    InitSignalHandler();
    InitCommands();
    RegisterDebugger();
  }
  virtual ~DebuggerBase() { UnregisterDebugger(); }

  // setters
  void set_prompt(std::string_view prompt) { prompt_ = prompt; }

 protected:
  // debugger command handler, returns true if need to quit CLI
  using CmdHandler = std::function<bool(std::istream &)>;
  // 'sigint' handler
  using SigIntHandler = std::function<void()>;

  // enter command line interface
  void EnterCLI();
  // register a command
  void RegisterCommand(std::string_view cmd, std::string_view abbr,
                       CmdHandler handler, std::string_view args,
                       std::string_view description,
                       std::string_view details);

  // setters
  void set_sigint_handler(SigIntHandler handler) {
    sigint_handler_ = handler;
  }

 private:
  // command line information
  struct CmdInfo {
    std::string_view name;
    std::string abbr;
    CmdHandler handler;
    std::string args;
    std::string description;
    std::string details;
  };

  // 'sigint' handler
  static void SignalHandler(int sig);
  // initialize 'sigint' handler
  static void InitSignalHandler();
  // register the current debugger
  void RegisterDebugger();
  // unregister the current debugger
  void UnregisterDebugger();

  // initialize command map
  void InitCommands();
  // get command info by name or abbreviation
  // show error message when command not found
  const CmdInfo *GetCommandInfo(const std::string &cmd) const;
  // parse command line, returns true if need to quit CLI
  bool ParseCommand(std::istream &is);
  // 'help' command
  bool ShowHelpInfo(std::istream &is);

  // flag for signal handler register status
  static volatile std::sig_atomic_t sig_registered_;
  // flag for enable/disable signal handler
  static volatile std::sig_atomic_t sig_disabled_;
  // all active debugger instances
  static std::unordered_set<DebuggerBase *> dbg_insts_;
  // 'sigint' handler
  SigIntHandler sigint_handler_;

  // prompt of command line interface
  std::string prompt_;
  // all registered commands
  // order should be maintained because we will
  // traverse it and show some messages
  std::map<std::string, CmdInfo> cmds_;
  // abbreviation of commands
  std::unordered_map<std::string_view, const CmdInfo *> abbrs_;
};

#endif  // MINIVM_DEBUGGER_DEBUGGER_H_
