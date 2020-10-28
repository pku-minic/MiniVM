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
#include "front/wrapper.h"
#include "vm/vm.h"
#include "vmconf.h"

using namespace std;

namespace {

xstl::ArgParser GetArgp() {
  xstl::ArgParser argp;
  argp.AddArgument<string>("input", "input Eeyore/Tigger IR file");
  argp.AddOption<bool>("help", "h", "show this message", false);
  argp.AddOption<bool>("version", "v", "show version info", false);
  argp.AddOption<bool>("tigger", "t", "run in Tigger mode", false);
  argp.AddOption<bool>("debug", "d", "enable debugger", false);
  argp.AddOption<string>("output", "o", "output file, default to stdout",
                         "");
  argp.AddOption<bool>("dump-gopher", "dg", "dump Gopher to output",
                       false);
  argp.AddOption<bool>("dump-bytecode", "db", "dump bytecode to output",
                       false);
  return argp;
}

void PrintVersion() {
  cout << APP_NAME << " version " << APP_VERSION << endl;
  cout << "MiniVM is a virtual machine for interpreting ";
  cout << "Eeyore/Tigger IR, " << endl;
  cout << "which is designed for PKU compiler course.";
  cout << endl << endl;
  cout << "Copyright (C) 2010-2020 MaxXing. License GPLv3.";
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

optional<VMOpr> RunEeyore(xstl::ArgParser &argp, string_view file,
                          ostream &os) {
  SymbolPool symbols;
  VMInstContainer cont(symbols);
  // parse input file
  if (!ParseEeyore(file, cont)) return {};
  if (argp.GetValue<bool>("dump-gopher")) {
    cont.Dump(os);
    return 0;
  }
  // run MiniVM
  VM vm(symbols, cont);
  InitEeyoreVM(vm);
  return vm.Run();
}

optional<VMOpr> RunTigger(xstl::ArgParser &argp, string_view file,
                          ostream &os) {
  SymbolPool symbols;
  VMInstContainer cont(symbols);
  // parse input file
  if (!ParseTigger(file, cont)) return {};
  if (argp.GetValue<bool>("dump-gopher")) {
    cont.Dump(os);
    return 0;
  }
  // run MiniVM
  VM vm(symbols, cont);
  InitTiggerVM(vm);
  return vm.Run();
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
  return ret ? *ret : -1;
}
