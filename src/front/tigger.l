%option yylineno
%option noyywrap
%option nounput
%option noinput
%option never-interactive

%{

#include <string_view>
#include <cstdlib>
#include <cassert>

#include "front/token.h"
#include "front/strpool.h"

#include "tigger.parser.hpp"

// get 'op_val' of the specific operator
TokenOp GetOpVal(std::string_view op);
// get 'reg_val' of the specific register
TokenReg GetRegVal(std::string_view reg);

%}

WhiteSp     [ \t\r\v\f]
Comment     \/\/.*
Label       l[0-9]+
Function    f_[_a-zA-Z][_a-zA-Z0-9]*
Num         -?[1-9][0-9]*|0
Variable      v[0-9]+
Register    x0|s[0-9]|s1[01]|t[0-6]|a[0-7]
Op          [\+\-\*\/\%\!]
LogicOp     !=|==|>|<|>=|<=|\|\||&&

%%

{WhiteSp}   { /* ignore white spaces */ }
{Comment}   { /* ignore line comments */ }

"\n"        { tigger_lloc.first_line = yylineno;
              tigger_lloc.last_line = yylineno;
              return EOL; }

"if"        { return IF; }
"goto"      { return GOTO; }
"call"      { return CALL; }
"return"    { return RETURN; }
"end"       { return END; }
"load"      { return LOAD; }
"store"     { return STORE; }
"loadaddr"  { return LOADADDR; }
"malloc"    { return MALLOC; }

{Label}     { tigger_lval.str_val = NewStr(yytext); return LABEL; }
{Function}  { tigger_lval.str_val = NewStr(yytext); return FUNCTION; }
{Variable}  { tigger_lval.str_val = NewStr(yytext); return VARIABLE; }
{Num}       { tigger_lval.int_val = std::atoi(yytext); return NUM; }
[\[\]:=]    { return yytext[0]; }
{Register}  { tigger_lval.reg_val = GetRegVal(yytext); return REG; }
{Op}        { tigger_lval.op_val = GetOpVal(yytext); return OP; }
{LogicOp}   { tigger_lval.op_val = GetOpVal(yytext); return LOGICOP; }

%%

TokenOp GetOpVal(std::string_view op) {
  const char *kOpStr[] = {TOKEN_OPERATORS(TOKEN_EXPAND_SECOND)};
  int op_val = 0;
  for (const auto &i : kOpStr) {
    if (op == i) return static_cast<TokenOp>(op_val);
    ++op_val;
  }
  assert(false);
  return TokenOp::Add;
}

TokenReg GetRegVal(std::string_view reg) {
  const char *kRegStr[] = {TOKEN_REGISTERS(TOKEN_EXPAND_SECOND)};
  int reg_val = 0;
  for (const auto &i : kRegStr) {
    if (reg == i) return static_cast<TokenReg>(reg_val);
    ++reg_val;
  }
  assert(false);
  return TokenReg::X0;
}
