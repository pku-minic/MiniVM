#ifndef MINIVM_VMCONF_H_
#define MINIVM_VMCONF_H_

// MiniVM configuration

#include <functional>

#include "vm/vm.h"

// type definition of VM initializer
using VMInit = std::function<void(minivm::vm::VM &)>;

// initialize a Eeyore mode MiniVM instance
void InitEeyoreVM(minivm::vm::VM &vm);
// initialize a Tigger mode MiniVM instance
void InitTiggerVM(minivm::vm::VM &vm);

#endif  // MINIVM_VMCONF_H_
