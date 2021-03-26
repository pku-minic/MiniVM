#ifndef MINIVM_DEBUGGER_EXPREVAL_H_
#define MINIVM_DEBUGGER_EXPREVAL_H_

#include <algorithm>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>

#include "xstl/guard.h"

// base class of expression evaluator
template <typename ValType>
class ExprEvaluatorBase {
 public:
  // 'ValType' must be an integral
  static_assert(std::is_integral_v<ValType>, "integral required");

  ExprEvaluatorBase() : next_id_(0) {}
  virtual ~ExprEvaluatorBase() = default;

  // evaluate expression with record
  std::optional<ValType> Eval(std::string_view expr) {
    return Eval(expr, true);
  }

  // evaluate expression
  std::optional<ValType> Eval(std::string_view expr, bool record) {
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
      expr.remove_prefix(
          std::min(expr.find_first_not_of(" "), expr.size()));
      auto pos = expr.find_last_not_of(" ");
      if (pos != expr.npos) expr.remove_suffix(expr.size() - pos - 1);
      // store to record
      records_.insert({next_id_++, std::string(expr)});
    }
    return val;
  }

  // evaluate expression by the specific record id
  std::optional<ValType> Eval(std::uint32_t id) {
    auto it = records_.find(id);
    if (it == records_.end()) return {};
    return Eval(it->second, false);
  }

  // show expression by the specific record id
  void PrintExpr(std::ostream &os, std::uint32_t id) {
    auto it = records_.find(id);
    assert(it != records_.end());
    os << it->second;
  }

  // remove the specific record
  void RemoveRecord(std::uint32_t id) { records_.erase(id); }

  // remove all records
  void Clear() { records_.clear(); }

  // getters
  // next record id
  std::uint32_t next_id() const { return next_id_; }

 protected:
  // get value of the specific symbol
  virtual std::optional<ValType> GetValueOfSym(std::string_view sym) = 0;
  // get value of the specific memory address
  virtual std::optional<ValType> GetValueOfAddr(ValType addr) = 0;

 private:
  /*
    EBNF of expressions:

    binary  ::= unary bin_op unray
    unary   ::= una_op value
    value   ::= NUM | SYMBOL | '(' binary ')'
  */

  enum class Token {
    End, Error, Char,
    Num, Symbol, ValRef,
    Operator,
  };

  enum class Operator {
    Add, Sub, Mul, Div, Mod,
    And, Or, Not, Xor, Shl, Shr,
    LogicAnd, LogicOr, LogicNot,
    Equal, NotEqual,
    LessThan, LessEqual, GreaterThan, GreaterEqual,
  };

  // print lexer error message to stderr
  Token LogLexerError(std::string_view msg) {
    std::cout << "ERROR (expr.lexer): " << msg << std::endl;
    return cur_token_ = Token::Error;
  }

  // print parser error message to stderr
  static std::optional<ValType> LogParserError(std::string_view msg) {
    std::cout << "ERROR (expr.parser): " << msg << std::endl;
    return {};
  }

  // check if specific character can appear in operators
  static bool IsOperatorChar(char c) {
    assert(c);
    constexpr const char kOpChar[] = "+-*/%&|~^!=<>";
    for (const auto &i : kOpChar) {
      if (c == i) return true;
    }
    return false;
  }

  // get the precedence of the specific operator
  static int GetOpPrec(Operator op) {
    constexpr int kOpPrec[] = {
        90, 90, 100, 100, 100, 50, 30, -1, 40, 80,
        80, 20, 10,  -1,  60,  60, 70, 70, 70, 70,
    };
    return kOpPrec[static_cast<int>(op)];
  }

  // perform binary operation
  static ValType CalcByOperator(Operator op, ValType lhs, ValType rhs) {
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

  // (lexer) read the next character
  void NextChar() { iss_ >> last_char_; }

  // (lexer) read the next token
  Token NextToken() {
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

  // (lexer) read numbers
  Token HandleNum() {
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

  // (lexer) read variable references
  Token HandleValRef() {
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

  // (lexer) read symbols
  Token HandleSymbol() {
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

  // (lexer) read operators
  Token HandleOperator() {
    std::string op;
    // get operator string
    do {
      op += last_char_;
      NextChar();
    } while (!iss_.eof() && IsOperatorChar(last_char_));
    // check is a valid operator
    constexpr std::string_view kOpList[] = {
        "+",  "-",  "*",  "/", "%",  "&",  "|", "~",  "^", "<<",
        ">>", "&&", "||", "!", "==", "!=", "<", "<=", ">", ">=",
    };
    for (int i = 0; i < sizeof(kOpList) / sizeof(std::string_view); ++i) {
      if (kOpList[i] == op) {
        op_val_ = static_cast<Operator>(i);
        return cur_token_ = Token::Operator;
      }
    }
    return LogLexerError("invalid operator");
  }

  // (parser) parse the current expression
  std::optional<ValType> Parse() {
    if (cur_token_ == Token::End) return {};
    return ParseBinary();
  }

  // (parser) parse binary expressions
  std::optional<ValType> ParseBinary() {
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

  // (parser) parse unary expressions
  std::optional<ValType> ParseUnary() {
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

  // (parser) parse values
  std::optional<ValType> ParseValue() {
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

  // all stored records
  std::unordered_map<std::uint32_t, std::string> records_;
  // next record id
  std::uint32_t next_id_;

  // lexer related stuffs
  //
  // current expression
  std::istringstream iss_;
  // last character
  char last_char_;
  // last character value
  char char_val_;
  // last number
  ValType num_val_;
  // last value reference
  std::uint32_t val_ref_;
  // last symbol value
  std::string sym_val_;
  // last operator
  Operator op_val_;

  // parser related stuffs
  //
  // current token
  Token cur_token_;
};

#endif  // MINIVM_DEBUGGER_EXPREVAL_H_
