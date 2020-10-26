#ifndef MINIVM_FRONT_WRAPPER_H_
#define MINIVM_FRONT_WRAPPER_H_

#include <string_view>

#include "vm/instcont.h"

// Eeyore parser
void ParseEeyore(std::string_view file, VMInstContainer &cont);

#endif  // MINIVM_FRONT_WRAPPER_H_
