#ifndef MINIVM_EEYORE_FRONT_TOKEN_H_
#define MINIVM_EEYORE_FRONT_TOKEN_H_

// all supported operators
#define TOKEN_OPERATORS(e)                                    \
  e(Add, "+") e(Sub, "-") e(Mul, "*") e(Div, "/") e(Mod, "%") \
  e(Not, "!") e(Ne, "!=") e(Eq, "==") e(Gt, ">") e(Lt, "<")   \
  e(Ge, ">=") e(Le, "<=") e(Or, "||") e(And, "&&")

// expand first element to comma-separated list
#define TOKEN_EXPAND_FIRST(i, ...)      i,
// expand second element to comma-separated list
#define TOKEN_EXPAND_SECOND(i, j, ...)  j,

enum class TokenOp { TOKEN_OPERATORS(TOKEN_EXPAND_FIRST) };

#endif  // MINIVM_EEYORE_FRONT_TOKEN_H_
