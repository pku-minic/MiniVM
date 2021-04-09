#include "front/wrapper.h"

#include <iostream>
#include <string>
#include <cstdio>

#include "front/strpool.h"
#include "xstl/style.h"

namespace {

// print file error to stderr
bool PrintFileError() {
  using namespace xstl;
  std::cerr << style("Br") << "file error: ";
  std::perror("");
  return false;
}

}  // namespace

using namespace minivm::vm;

// defined in Flex/Bison generated files
extern std::FILE *eeyore_in;
int eeyore_parse(void *cont);
extern std::FILE *tigger_in;
int tigger_parse(void *cont);

bool minivm::front::ParseEeyore(std::string_view file,
                                VMInstContainer &cont) {
  if (!(eeyore_in = std::fopen(std::string(file).c_str(), "r"))) {
    return PrintFileError();
  }
  auto ret = eeyore_parse(&cont);
  cont.SealContainer();
  std::fclose(eeyore_in);
  FreeAllStrs();
  return !ret;
}

bool minivm::front::ParseTigger(std::string_view file,
                                VMInstContainer &cont) {
  if (!(tigger_in = std::fopen(std::string(file).c_str(), "r"))) {
    return PrintFileError();
  }
  auto ret = tigger_parse(&cont);
  cont.SealContainer();
  std::fclose(tigger_in);
  FreeAllStrs();
  return !ret;
}
