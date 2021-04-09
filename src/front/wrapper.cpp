#include "front/wrapper.h"

#include <string>
#include <cstdio>

#include "front/strpool.h"

using namespace minivm::vm;

// defined in Flex/Bison generated files
extern std::FILE *eeyore_in;
int eeyore_parse(void *cont);
extern std::FILE *tigger_in;
int tigger_parse(void *cont);

bool minivm::front::ParseEeyore(std::string_view file,
                                VMInstContainer &cont) {
  eeyore_in = fopen(std::string(file).c_str(), "r");
  auto ret = eeyore_parse(&cont);
  cont.SealContainer();
  fclose(eeyore_in);
  FreeAllStrs();
  return !ret;
}

bool minivm::front::ParseTigger(std::string_view file,
                                VMInstContainer &cont) {
  tigger_in = fopen(std::string(file).c_str(), "r");
  auto ret = tigger_parse(&cont);
  cont.SealContainer();
  fclose(tigger_in);
  FreeAllStrs();
  return !ret;
}
