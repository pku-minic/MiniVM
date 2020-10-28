#ifndef MINIVM_FRONT_TOKEN_H_
#define MINIVM_FRONT_TOKEN_H_

// all supported operators
#define TOKEN_OPERATORS(e)                                    \
  e(Add, "+") e(Sub, "-") e(Mul, "*") e(Div, "/") e(Mod, "%") \
  e(Not, "!") e(Ne, "!=") e(Eq, "==") e(Gt, ">") e(Lt, "<")   \
  e(Ge, ">=") e(Le, "<=") e(Or, "||") e(And, "&&")
// all supported registers
#define TOKEN_REGISTERS(e)                                    \
  e(X0, "x0") e(S0, "s0") e(S1, "s1") e(S2, "s2") e(S3, "s3") \
  e(S4, "s4") e(S5, "s5") e(S6, "s6") e(S7, "s7") e(S8, "s8") \
  e(S9, "s9") e(S10, "s10") e(S11, "s11") e(T0, "t0")         \
  e(T1, "t1") e(T2, "t2") e(T3, "t3") e(T4, "t4") e(T5, "t5") \
  e(T6, "t6") e(A0, "a0") e(A1, "a1") e(A2, "a2") e(A3, "a3") \
  e(A4, "a4") e(A5, "a5") e(A6, "a6") e(A7, "a7")

// expand first element to comma-separated list
#define TOKEN_EXPAND_FIRST(i, ...)      i,
// expand second element to comma-separated list
#define TOKEN_EXPAND_SECOND(i, j, ...)  j,
// expand elements to ones
#define TOKEN_EXPAND_ONES(...)          1+
// count all elements
#define TOKEN_COUNT(e)                  (e(TOKEN_EXPAND_ONES)0)

enum class TokenOp { TOKEN_OPERATORS(TOKEN_EXPAND_FIRST) };
enum class TokenReg { TOKEN_REGISTERS(TOKEN_EXPAND_FIRST) };

#endif  // MINIVM_FRONT_TOKEN_H_
