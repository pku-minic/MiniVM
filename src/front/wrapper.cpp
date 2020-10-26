#include "front/wrapper.h"

#include <string>
#include <cstdio>

#include "front/strpool.h"

// defined in Flex/Bison generated files
extern std::FILE *eeyore_in;
int eeyore_parse(VMInstContainer &cont);

bool ParseEeyore(std::string_view file, VMInstContainer &cont) {
  eeyore_in = fopen(std::string(file).c_str(), "r");
  auto ret = eeyore_parse(cont);
  fclose(eeyore_in);
  FreeAllStrs();
  return !ret;
}
