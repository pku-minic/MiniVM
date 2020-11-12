#ifndef MINIVM_DEBUGGER_EXPREVAL_H_
#define MINIVM_DEBUGGER_EXPREVAL_H_

#include <optional>
#include <string_view>
#include <ostream>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cstdint>

template <typename ValType>
class ExprEvaluatorBase {
 public:
  ExprEvaluatorBase() : next_id_(0) {}

  // evaluate expression with record
  std::optional<ValType> Eval(std::string_view expr);
  // evaluate expression
  std::optional<ValType> Eval(std::string_view expr, bool record);
  // evaluate expression by the specific record id
  std::optional<ValType> Eval(std::uint32_t id);

  // show expression by the specific record id
  void PrintExpr(std::ostream &os, std::uint32_t id);
  // remove the specific record
  void RemoveRecord(std::uint32_t id);
  // remove all records
  void Clear();

  // getters
  // next record id
  std::uint32_t next_id() const { return next_id_; }

 protected:
  // get value of the specific symbol
  virtual std::optional<ValType> GetValueOfSym(std::string_view sym) = 0;
  // get value of the specific memory address
  virtual std::optional<ValType> GetValueOfAddr(ValType addr) = 0;

 private:
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

  // helper functions
  Token LogLexerError(std::string_view msg);
  std::optional<ValType> LogParserError(std::string_view msg);
  int GetOpPrec(Operator op);
  ValType CalcByOperator(Operator op, ValType lhs, ValType rhs);

  // lexer
  void NextChar() { iss_ >> last_char_; }
  Token NextToken();
  Token HandleNum();
  Token HandleValRef();
  Token HandleSymbol();
  Token HandleOperator();

  // parser
  std::optional<ValType> Parse();
  std::optional<ValType> ParseBinary();
  std::optional<ValType> ParseUnary();
  std::optional<ValType> ParseValue();

  // all stored records
  std::unordered_map<std::uint32_t, std::string> records_;
  // next record id
  std::uint32_t next_id_;

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

  // current token
  Token cur_token_;
};

#endif  // MINIVM_DEBUGGER_EXPREVAL_H_
