# A Brief Introduction to MiniVM

MiniVM is a virtual machine for interpreting Eeyore/Tigger IR, so its design is deeply influenced by Eeyore and Tigger.

The current version of MiniVM is implemented in C++ language, but in fact, there are no restrictions on the host language that implements MiniVM.

## Architecture of Virtual Machine

MiniVM is a stack-based virtual machine, which means most of the operands required by instructions come from the internal stack of the virtual machine. In details, MiniVM has the following built-in structures:

* **Operand stack**: storing operands or parameters.
* **Memory pool stack**: holding all dynamically allocated memory and return addresses for each active functions.
* **Static registers**: storing some static data, designed to execute Tigger IR.
* **Symbol pool**: holding all symbols, which can be variable names or external function names.
* **Instruction container**: holding all instructions and debugging information.
* **External function table**: holding all definitions of external functions.

Considering that static registers are closely related to the target architecture supported by Tigger, although the current Tigger IR only supports the RISC-V architecture, it may support more architectures in the future. Therefore, the count of the static registers should not be fixed.

The external function table can be modified before MiniVM starts. Developers can register any host language function to the table, and assign a symbol to it, in order to provide library functions such as `putint` for programs running in MiniVM.

## Instruction Definition

MiniVM can not directly execute Eeyore or Tigger, so there should be a front end to read Eeyore/Tigger source files, and generate instructions that can be recognized by MiniVM.

The structure of MiniVM instruction is defined as follows:

```
0   7 8       31
+----+---------+
| op |   imm   |
+----+---------+
```

As we can see, it consists of two field `op` and `imm`, the former is the opcode of instruction, and the latter is the operand, which can be:

* `sym`: symbol reference, which represents a symbol in the symbol pool.
* `imm`: 24-bit immediate.
* `pc`: absolute target address, used in control transfer instructions.
* `reg`: id of static register.

MiniVM supports the following instructions:

* **Memory allocation**: Var, Arr.
* **Load and store**: Ld, LdVar, LdReg, LdAddr, St, StVar, StReg, Imm, ImmHi.
* **Control transfer**: Bnz, Jmp.
* **Function call**: Call, CallExt, Ret, Param.
* **Logical operations**: LNot, LAnd, LOr.
* **Comparisons**: Eq, Ne, Gt, Lt, Ge, Le.
* **Arithmetic operations**: Neg, Add, Sub, Mul, Div, Mod.

Details as follows:

> **Note**
>
> In the `Stack operand` column, the rightmost element is at the top of the stack.

| Opcode  | Operand   | Stack Operand     | Description                                 |
| ---     | ---       | ---               | ---                                         |
| Var     | `sym`     | N/A               | allocate memory for variable `sym`          |
| Arr     | `sym`     | size (in byte)    | allocate memory for array `sym`             |
| Ld      | N/A       | addr              | load 32-bit data from addr to stack         |
| LdVar   | `sym`     | N/A               | load 32-bit data from `sym`'s addr to stack |
| LdReg   | `reg`     | N/A               | load 32-bit data from `reg` to stack        |
| LdAddr  | `sym`     | offset (in byte)  | load address of `sym`+offset to stack       |
| St      | N/A       | addr              | store top of stack to addr                  |
| StVar   | `sym`     | N/A               | store top of stack to `sym`'s addr          |
| StReg   | `reg`     | N/A               | store top of stack to `reg`                 |
| Imm     | `imm`     | N/A               | load 24-bit `imm` to stack                  |
| ImmHi   | `imm`     | N/A               | load `imm`&8 to upper 8-bit of top of stack |
| Bnz     | `pc`      | cond              | jump to `pc` if cond is not zero            |
| Jmp     | `pc`      | N/A               | jump to `pc`                                |
| Call    | `pc`      | N/A               | call function at `pc`                       |
| CallExt | `sym`     | N/A               | call external function `sym`                |
| Ret     | N/A       | N/A               | return from a function call                 |
| Param   | N/A       | param             | push param to stack                         |
| Break   | N/A       | N/A               | breakpoint, inserted by debugger            |
| LNot    | N/A       | opr               | perform logical negation                    |
| LAnd    | N/A       | lhs, rhs          | perform logical AND operation               |
| LOr     | N/A       | lhs, rhs          | perform logical OR operation                |
| Eq      | N/A       | lhs, rhs          | push (lhs == rhs) to stack                  |
| Ne      | N/A       | lhs, rhs          | push (lhs != rhs) to stack                  |
| Gt      | N/A       | lhs, rhs          | push (lhs > rhs) to stack                   |
| Lt      | N/A       | lhs, rhs          | push (lhs < rhs) to stack                   |
| Ge      | N/A       | lhs, rhs          | push (lhs >= rhs) to stack                  |
| Le      | N/A       | lhs, rhs          | push (lhs <= rhs) to stack                  |
| Neg     | N/A       | opr               | perform negation                            |
| Add     | N/A       | lhs, rhs          | perform addition                            |
| Sub     | N/A       | lhs, rhs          | perform subtraction                         |
| Mul     | N/A       | lhs, rhs          | perform multiplication                      |
| Div     | N/A       | lhs, rhs          | perform division                            |
| Mod     | N/A       | lhs, rhs          | Perform modulo operation                    |

## Calling Conventions

When executing a `Call`/`CallExt` instruction, MiniVM will:

1. Create a new memory pool, and push it to the memory pool stack.
2. Check the operand stack, if there are any values in it, pop them and add them to the memory pool, then assign symbol to it, such as `p0`, `p1`, etc.
3. Jump to target pc address, or call the specific external function.

On the contrast, when executing a `Ret` instruction, MiniVM will:

* If the size of the memory pool stack is 1, it means that instructions are currently being executed in the global environment, just stop the execution.
* Otherwise, pop a memory pool from the top of the stack and release it, and then jump to the return address.

So what about the details of external functions? We make the following conventions:

### Form of External Functions

MiniVM can accept external functions in the following form:

```
func externalFunc(vm_instance: MiniVM): Unit
```

Therefore, external functions can access the current memory pool and all static registers in MiniVM.

### Passing Parameters

To be compatible with Eeyore and Tigger, MiniVM supports two methods for passing parameters:

1. Store parameter #1, parameter #2... to function's memory pool with the symbol `p1`, `p2`..., or
2. Store parameters to the specific static registers or an data block with symbol `$frame` in memory pool, depends on the target architecture of Tigger.

MiniVM ***must ensure***:

* Before the `Call`/`CallExt` instructions are executed, there is no value in the operand stack, except for function parameters.
* When executing Eeyore generated instructions, there are no data blocks with symbol `$frame` in the current memory pool.
* When executing Tigger generated instructions, there must be a data blocks with symbol `$frame` in the current memory pool, and, all `$frame`s should be placed consecutively in the same memory area.

Considering the impact on performance, we strongly recommend that developers should create two versions of external functions for MiniVM running in Eeyore mode and Tigger mode.

## Extending the Instruction Set

> Still WIP.

## Debugger of MiniVM

> Still WIP.
