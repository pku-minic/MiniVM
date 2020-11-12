#include "debugger/debugger.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <utility>
#include <cstddef>
#include <cstdlib>
#include <cassert>

#include "readline/readline.h"
#include "readline/history.h"

namespace {

//

}  // namespace

void DebuggerBase::InitCommands() {
  // register 'help' command
  RegisterCommand(
      "help", "", [this](std::istream &is) { return ShowHelpInfo(is); },
      "[CMD]", "show help message of CMD",
      "Show a list of all debugger commands, or give details about a "
      "specific command.");
}

const DebuggerBase::CmdInfo *DebuggerBase::GetCommandInfo(
    const std::string &cmd) const {
  // try to find in registered commands
  auto it = cmds_.find(cmd);
  if (it != cmds_.end()) {
    return &it->second;
  }
  else {
    // try to find in abbreviations
    auto it = abbrs_.find(cmd);
    if (it != abbrs_.end()) {
      return it->second;
    }
    else {
      // command not found
      std::cout << "unknown command, run 'help' to see command list"
                << std::endl;
      return nullptr;
    }
  }
}

bool DebuggerBase::ParseCommand(std::istream &is) {
  // read command
  std::string cmd;
  is >> cmd;
  // call handler
  auto info = GetCommandInfo(cmd);
  return info ? info->handler(is) : false;
}

bool DebuggerBase::ShowHelpInfo(std::istream &is) {
  if (is.eof()) {
    // show brief help messages of all commands
    std::cout << "Debugger commands:" << std::endl;
    // get the length of the longest command
    std::size_t cmd_len = 0, args_len = 0;
    for (const auto &[name, info] : cmds_) {
      std::size_t cur_len = name.size() + info.abbr.size();
      if (cur_len > cmd_len) cmd_len = cur_len;
      if (info.args.size() > args_len) args_len = info.args.size();
    }
    // show messages
    for (const auto &[name, info] : cmds_) {
      auto cmd = name;
      if (!info.abbr.empty()) cmd += '/' + info.abbr;
      std::cout << "  " << std::setw(cmd_len + 2) << std::setfill(' ')
                << cmd << std::setw(args_len + 2) << info.args;
      std::cout << "--- " << info.description << std::endl;
    }
  }
  else {
    // show detailed help messages of the specific command
    std::string cmd;
    is >> cmd;
    if (auto info = GetCommandInfo(cmd)) {
      if (!info->abbr.empty()) cmd += '/' + info->abbr;
      std::cout << "Syntax: " << cmd << " " << info->args << std::endl;
      std::cout << "  " << info->details << std::endl;
    }
  }
  return false;
}

void DebuggerBase::EnterCLI() {
  bool quit = false;
  std::istringstream iss;
  while (!quit) {
    std::cout << std::endl;
    // read line from terminal
    auto line = readline(prompt_.c_str());
    if (!line) {
      // EOF, just exit
      std::cout << "quit" << std::endl;
      std::exit(0);
    }
    if (*line) {
      // add current line to history
      add_history(line);
      // reset string stream
      iss.str(line);
      iss.clear();
      // parse command
      quit = ParseCommand(iss);
    }
    // free line buffer
    std::free(line);
  }
}

void DebuggerBase::RegisterCommand(std::string_view cmd,
                                   std::string_view abbr,
                                   CmdHandler handler,
                                   std::string_view args,
                                   std::string_view description,
                                   std::string_view details) {
  CmdInfo info = {std::string(abbr), handler, std::string(args),
                  std::string(description), std::string(details)};
  // insert to command map
  auto ret = cmds_.insert({std::string(cmd), std::move(info)});
  assert(ret.second);
  // insert to abbreviation map
  if (!abbr.empty()) {
    const auto &it = ret.first;
    auto succ = abbrs_.insert({it->first, &it->second}).second;
    assert(succ);
    static_cast<void>(succ);
  }
}
