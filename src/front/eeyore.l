%option yylineno
%option noyywrap
%option nounput
%option noinput
%option never-interactive

%{

#include <iostream>
#include <string_view>
#include <cstdlib>
#include <cassert>

#include "front/token.h"
#include "front/strpool.h"

#include "eeyore.parser.hpp"

// get 'op_val' of the specific operator
TokenOp GetOpVal(std::string_view op);

%}

WhiteSp     [ \t\r]
Comment     \/\/.*
Label       l[0-9]+
Function    f_[_a-zA-Z][_a-zA-Z0-9]*
Num         -?[1-9][0-9]*|0
Symbol      [Ttp][0-9]+
Op          [\+\-\*\/\%\!]
LogicOp     !=|==|>|<|>=|<=|\|\||&&
AddrOfOp    &

%%

{WhiteSp}   { /* ignore white spaces */ }
{Comment}   { /* ignore line comments */ }

"\n"        { return EOL; }

"var"       { return VAR; }
"if"        { return IF; }
"goto"      { return GOTO; }
"param"     { return PARAM; }
"call"      { return CALL; }
"return"    { return RETURN; }
"end"       { return END; }

{Label}     { eeyore_lval.str_val = NewStr(yytext); return LABEL; }
{Function}  { eeyore_lval.str_val = NewStr(yytext); return FUNCTION; }
{Symbol}    { eeyore_lval.str_val = NewStr(yytext); return SYMBOL; }
{Num}       { eeyore_lval.int_val = std::atoi(yytext); return NUM; }
[\[\]:=]    { return yytext[0]; }
{Op}        { eeyore_lval.op_val = GetOpVal(yytext); return OP; }
{LogicOp}   { eeyore_lval.op_val = GetOpVal(yytext); return LOGICOP; }
{AddrOfOp}  { return ADDROFOP; }

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
