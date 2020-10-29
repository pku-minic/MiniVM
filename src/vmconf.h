#ifndef MINIVM_VMCONF_H_
#define MINIVM_VMCONF_H_

// MiniVM configuration

#include "vm/vm.h"

// initialize a Eeyore mode MiniVM instance
void InitEeyoreVM(VM &vm);
// initialize a Tigger mode MiniVM instance
void InitTiggerVM(VM &vm);

#endif  // MINIVM_VMCONF_H_
