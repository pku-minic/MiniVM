#include <iostream>
#include <string>
#include <optional>
#include <string_view>
#include <ostream>
#include <fstream>
#include <cstdlib>

#include "xstl/argparse.h"

#include "version.h"
#include "vm/symbol.h"
#include "vm/instcont.h"
#include "back/c/codegen.h"
#include "front/wrapper.h"
#include "vm/vm.h"
#include "vmconf.h"
#ifndef NO_DEBUGGER
#include "debugger/minidbg/minidbg.h"
#endif

using namespace std;
using namespace minivm::vm;
using namespace minivm::back::c;
using namespace minivm::front;
using namespace minivm::debugger::minidbg;

namespace {

xstl::ArgParser GetArgp() {
  xstl::ArgParser argp;
  argp.AddArgument<string>("input", "input Eeyore/Tigger IR file");
  argp.AddOption<bool>("help", "h", "show this message", false);
  argp.AddOption<bool>("version", "v", "show version info", false);
  argp.AddOption<bool>("tigger", "t", "run in Tigger mode", false);
#ifndef NO_DEBUGGER
  argp.AddOption<bool>("debug", "d", "enable debugger", false);
#endif
  argp.AddOption<string>("output", "o", "output file, default to stdout",
                         "");
  argp.AddOption<bool>("dump-gopher", "dg", "dump Gopher to output",
                       false);
  // TODO: implement this option
  argp.AddOption<bool>("dump-bytecode", "db", "dump bytecode to output",
                       false);
  argp.AddOption<bool>("compile", "c", "compile input file to C code",
                       false);
  return argp;
}

void PrintVersion() {
  cout << APP_NAME << " version " << APP_VERSION << endl << endl;
  cout << "MiniVM is a virtual machine for interpreting ";
  cout << "Eeyore/Tigger IR," << endl;
  cout << "which is designed for PKU compiler course.";
  cout << endl << endl;
  cout << "Copyright (C) 2010-2021 MaxXing. License GPLv3.";
  cout << endl;
}

void ParseArgument(xstl::ArgParser &argp, int argc, const char *argv[]) {
  auto ret = argp.Parse(argc, argv);
  // check if need to exit program
  if (argp.GetValue<bool>("help")) {
    argp.PrintHelp();
    exit(0);
  }
  else if (argp.GetValue<bool>("version")) {
    PrintVersion();
    exit(0);
  }
  else if (!ret) {
    cerr << "invalid input, run '";
    cerr << argp.program_name() << " -h' for help" << endl;
    exit(1);
  }
}

optional<VMOpr> RunVM(xstl::ArgParser &argp, string_view file, ostream &os,
                      Parser parser, VMInit vm_init, bool tigger_mode) {
  SymbolPool symbols;
  VMInstContainer cont(symbols, file);
  // parse input file
  if (!parser(file, cont)) return {};
  if (argp.GetValue<bool>("dump-gopher")) {
    // dump Gopher
    cont.Dump(os);
    return 0;
  }
  else if (argp.GetValue<bool>("compile")) {
    // compile to C code
    CCodeGen gen(cont, tigger_mode);
    gen.Generate();
    if (gen.has_error()) return {};
    gen.Dump(os);
    return 0;
  }
  // run MiniVM
  std::optional<VMOpr> ret;
  VM vm(symbols, cont);
  vm_init(vm);
#ifdef NO_DEBUGGER
  ret = vm.Run();
#else
  if (argp.GetValue<bool>("debug")) {
    // debug mode
    MiniDebugger debugger(vm);
    PrintVersion();
    ret = vm.Run();
    // print exit code
    if (!ret) {
      std::cout << "VM instance ended with error code ";
      std::cout << vm.error_code() << std::endl;
    }
    else {
      std::cout << "VM instance exited with code " << *ret << std::endl;
    }
  }
  else {
    ret = vm.Run();
  }
#endif
  return ret ? *ret : static_cast<VMOpr>(vm.error_code());
}

optional<VMOpr> RunEeyore(xstl::ArgParser &argp, string_view file,
                          ostream &os) {
  return RunVM(argp, file, os, ParseEeyore, InitEeyoreVM, false);
}

optional<VMOpr> RunTigger(xstl::ArgParser &argp, string_view file,
                          ostream &os) {
  return RunVM(argp, file, os, ParseTigger, InitTiggerVM, true);
}

}  // namespace

int main(int argc, const char *argv[]) {
  // set up argument parser & parse argument
  auto argp = GetArgp();
  ParseArgument(argp, argc, argv);

  // get input file name & output stream
  auto in_file = argp.GetValue<string>("input");
  auto out_file = argp.GetValue<string>("output");
  ofstream ofs;
  if (!out_file.empty()) ofs.open(out_file);
  auto &os = out_file.empty() ? cout : ofs;

  // parse & run
  optional<VMOpr> ret;
  if (argp.GetValue<bool>("tigger")) {
    ret = RunTigger(argp, in_file, os);
  }
  else {
    ret = RunEeyore(argp, in_file, os);
  }
  return ret ? *ret : kVMErrorVMIrrelevant;
}
