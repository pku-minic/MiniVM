#include "debugger/expreval.h"

#include <iostream>
#include <stack>
#include <utility>
#include <cassert>
#include <cctype>
#include <cstdlib>

#include "xstl/guard.h"

namespace {

/*

EBNF of expressions:

binary  ::= unary bin_op unray
unary   ::= una_op value
value   ::= NUM | SYMBOL | '(' binary ')'

*/

// all supported operators
constexpr std::string_view kOpList[] = {
    "+",  "-",  "*",  "/", "%",  "&",  "|", "~",  "^", "<<",
    ">>", "&&", "||", "!", "==", "!=", "<", "<=", ">", ">=",
};

// precedence of operators
constexpr int kOpPrec[] = {
    90, 90, 100, 100, 100, 50, 30, -1, 40, 80,
    80, 20, 10,  -1,  60,  60, 70, 70, 70, 70,
};

// check if specific character can appear in operators
inline bool IsOperatorChar(char c) {
  assert(c);
  constexpr const char kOpChar[] = "+-*/%&|~^!=<>";
  for (const auto &i : kOpChar) {
    if (c == i) return true;
  }
  return false;
}

}  // namespace

template <typename ValType>
ExprEvaluatorBase<ValType>::Token ExprEvaluatorBase<ValType>::LogLexerError(
    std::string_view msg) {
  std::cout << "ERROR (expr.lexer): " << msg << std::endl;
  return cur_token_ = Token::Error;
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::LogParserError(
    std::string_view msg) {
  std::cout << "ERROR (expr.parser): " << msg << std::endl;
  return {};
}

template <typename ValType>
int ExprEvaluatorBase<ValType>::GetOpPrec(Operator op) {
  return kOpPrec[static_cast<int>(op)];
}

template <typename ValType>
ValType ExprEvaluatorBase<ValType>::CalcByOperator(Operator op, ValType lhs,
                                                   ValType rhs) {
  switch (op) {
    case Operator::Add:           return lhs + rhs;
    case Operator::Sub:           return lhs - rhs;
    case Operator::Mul:           return lhs * rhs;
    case Operator::Div:           return lhs / rhs;
    case Operator::And:           return lhs & rhs;
    case Operator::Or:            return lhs | rhs;
    case Operator::Xor:           return lhs ^ rhs;
    case Operator::Shl:           return lhs << rhs;
    case Operator::Shr:           return lhs >> rhs;
    case Operator::LogicAnd:      return lhs && rhs;
    case Operator::LogicOr:       return lhs || rhs;
    case Operator::Equal:         return lhs == rhs;
    case Operator::NotEqual:      return lhs != rhs;
    case Operator::LessThan:      return lhs < rhs;
    case Operator::LessEqual:     return lhs <= rhs;
    case Operator::GreaterThan:   return lhs > rhs;
    case Operator::GreaterEqual:  return lhs >= rhs;
    default: assert(false); return 0;
  }
}

template <typename ValType>
ExprEvaluatorBase<ValType>::Token ExprEvaluatorBase<ValType>::NextToken() {
  // skip spaces
  while (!iss_.eof() && std::isspace(last_char_)) NextChar();
  // end of stream
  if (iss_.eof()) return cur_token_ = Token::End;
  // numbers
  if (std::isdigit(last_char_)) return HandleNum();
  // value references or symbols
  if (last_char_ == '$') return HandleValRef();
  // symbols
  if (std::isalpha(last_char_)) return HandleSymbol();
  // operators
  if (IsOperatorChar(last_char_)) return HandleOperator();
  // other characters
  char_val_ = last_char_;
  NextChar();
  return cur_token_ = Token::Char;
}

template <typename ValType>
ExprEvaluatorBase<ValType>::Token ExprEvaluatorBase<ValType>::HandleNum() {
  std::string num;
  bool is_hex = false;
  // check if is hexadecimal number
  if (last_char_ == '0') {
    NextChar();
    if (std::tolower(last_char_) == 'x') {
      // is hexadecimal
      is_hex = true;
      NextChar();
    }
    else if (!std::isdigit(last_char_)) {
      // just zero
      num_val_ = 0;
      return cur_token_ = Token::Num;
    }
  }
  // read number string
  while (!iss_.eof() && std::isxdigit(last_char_)) {
    num += last_char_;
    NextChar();
  }
  // convert to number
  char *end_pos;
  num_val_ = std::strtol(num.c_str(), &end_pos, is_hex ? 16 : 10);
  if (*end_pos) return LogLexerError("invalid number literal");
  return cur_token_ = Token::Num;
}

template <typename ValType>
ExprEvaluatorBase<ValType>::Token
ExprEvaluatorBase<ValType>::HandleValRef() {
  std::string ref;
  // eat '$'
  NextChar();
  if (std::isalpha(last_char_)) {
    // get symbol name
    ref += '$';
    while (!iss_.eof() && std::isalnum(last_char_)) {
      ref += last_char_;
      NextChar();
    }
    // update last symbol
    sym_val_ = ref;
    return cur_token_ = Token::Symbol;
  }
  else if (std::isdigit(last_char_)) {
    // get value reference number (record id)
    while (!iss_.eof() && std::isdigit(last_char_)) {
      ref += last_char_;
      NextChar();
    }
    // convert to number
    char *end_pos;
    val_ref_ = std::strtol(ref.c_str(), &end_pos, 10);
    if (*end_pos || records_.find(val_ref_) == records_.end()) {
      return LogLexerError("invalid value reference");
    }
    return cur_token_ = Token::ValRef;
  }
  else {
    return LogLexerError("invalid '$' expression");
  }
}

template <typename ValType>
ExprEvaluatorBase<ValType>::Token
ExprEvaluatorBase<ValType>::HandleSymbol() {
  std::string sym;
  // get symbol string
  do {
    sym += last_char_;
    NextChar();
  } while (!iss_.eof() && std::isalnum(last_char_));
  // update last symbol
  sym_val_ = sym;
  return cur_token_ = Token::Symbol;
}

template <typename ValType>
ExprEvaluatorBase<ValType>::Token
ExprEvaluatorBase<ValType>::HandleOperator() {
  std::string op;
  // get operator string
  do {
    op += last_char_;
    NextChar();
  } while (!iss_.eof() && IsOperatorChar(last_char_));
  // check is a valid operator
  for (int i = 0; i < sizeof(kOpList) / sizeof(std::string_view); ++i) {
    if (kOpList[i] == op) {
      op_val_ = static_cast<Operator>(i);
      return cur_token_ = Token::Operator;
    }
  }
  return LogLexerError("invalid operator");
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::Parse() {
  if (cur_token_ == Token::End) return {};
  return ParseBinary();
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::ParseBinary() {
  std::stack<ValType> oprs;
  std::stack<Operator> ops;
  // get the first value
  auto val = ParseUnary();
  if (!val) return {};
  oprs.push(*val);
  // calculate using stack
  while (cur_token_ == Token::Operator) {
    // get operator
    auto op = op_val_;
    if (GetOpPrec(op) < 0) break;
    NextToken();
    // handle operator
    while (!ops.empty() && GetOpPrec(ops.top()) >= GetOpPrec(op)) {
      // get current operator & operand
      auto cur_op = ops.top();
      ops.pop();
      auto rhs = oprs.top();
      oprs.pop();
      auto lhs = oprs.top();
      oprs.pop();
      // calculate
      oprs.push(CalcByOperator(cur_op, lhs, rhs));
    }
    // push & get next value
    ops.push(op);
    if (!(val = ParseUnary())) return false;
    oprs.push(*val);
  }
  // clear stacks
  while (!ops.empty()) {
    auto cur_op = ops.top();
    ops.pop();
    auto rhs = oprs.top();
    oprs.pop();
    auto lhs = oprs.top();
    oprs.pop();
    oprs.push(CalcByOperator(cur_op, lhs, rhs));
  }
  return oprs.top();
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::ParseUnary() {
  // check if need to get operator
  if (cur_token_ == Token::Operator) {
    auto op = op_val_;
    NextToken();
    // get operand
    auto opr = ParseUnary();
    if (!opr) return {};
    // calculate
    switch (op) {
      case Operator::Add: return *opr;
      case Operator::Sub: return -*opr;
      case Operator::LogicNot: return !*opr;
      case Operator::Not: return ~*opr;
      case Operator::Mul: return GetValueOfAddr(*opr);
      default: return LogParserError("invalid unary operator");
    }
  }
  else {
    return ParseValue();
  }
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::ParseValue() {
  xstl::Guard guard([this] { NextToken(); });
  switch (cur_token_) {
    case Token::Num: {
      // just number
      return num_val_;
    }
    case Token::Symbol: {
      // get value of the symbol
      return GetValueOfSym(sym_val_);
    }
    case Token::ValRef: {
      // store current state
      auto iss = std::move(iss_);
      auto last_char = last_char_;
      auto cur_token = cur_token_;
      // evaluate record
      auto val = Eval(val_ref_);
      assert(val);
      // restore current state
      iss_ = std::move(iss);
      last_char_ = last_char;
      cur_token_ = cur_token;
      return val;
    }
    case Token::Char: {
      // check & eat '('
      if (char_val_ != '(') return LogParserError("expected '('");
      NextToken();
      // parse inner binary expression
      auto val = ParseBinary();
      if (!val) return {};
      // check ')'
      if (cur_token_ != Token::Char || char_val_ != ')') {
        return LogParserError("expected ')'");
      }
      return val;
    }
    default: return LogParserError("invalid value");
  }
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::Eval(
    std::string_view expr) {
  return Eval(expr, true);
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::Eval(
    std::string_view expr, bool record) {
  // reset string stream
  iss_.str({expr.data(), expr.size()});
  iss_.clear();
  last_char_ = ' ';
  // call lexer & parser
  NextToken();
  auto val = Parse();
  if (!val) return {};
  // record expression
  if (record) {
    // trim expression string
    expr.remove_prefix(std::min(expr.find_first_not_of(" "), expr.size()));
    auto pos = expr.find_last_not_of(" ");
    if (pos != expr.npos) expr.remove_suffix(expr.size() - pos - 1);
    // store to record
    records_.insert({next_id_++, std::string(expr)});
  }
  return val;
}

template <typename ValType>
std::optional<ValType> ExprEvaluatorBase<ValType>::Eval(std::uint32_t id) {
  auto it = records_.find(id);
  if (it == records_.end()) return false;
  return Eval(it->second, false);
}

template <typename ValType>
void ExprEvaluatorBase<ValType>::PrintExpr(std::ostream &os,
                                           std::uint32_t id) {
  auto it = records_.find(id);
  assert(it != records_.end());
  os << it->second;
}

template <typename ValType>
void ExprEvaluatorBase<ValType>::RemoveRecord(std::uint32_t id) {
  records_.erase(id);
}

template <typename ValType>
void ExprEvaluatorBase<ValType>::Clear() {
  records_.clear();
}
