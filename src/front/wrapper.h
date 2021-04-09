#ifndef MINIVM_FRONT_WRAPPER_H_
#define MINIVM_FRONT_WRAPPER_H_

#include <string_view>
#include <functional>

#include "vm/instcont.h"

namespace minivm::front {

// type definition of parser function
using Parser = std::function<bool(std::string_view, vm::VMInstContainer &)>;

// Eeyore parser
// returns false if parsing failed
bool ParseEeyore(std::string_view file, vm::VMInstContainer &cont);

// Tigger parser
// returns false if parsing failed
bool ParseTigger(std::string_view file, vm::VMInstContainer &cont);

}  // namespace minivm::front

#endif  // MINIVM_FRONT_WRAPPER_H_
