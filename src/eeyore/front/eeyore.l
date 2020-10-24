%option yylineno
%option noyywrap
%option never-interactive

%{

#include <iostream>
#include <string_view>
#include <cstdlib>
#include <cassert>

#include "eeyore.tab.hpp"
#include "front/token.h"

#ifdef NDEBUG
#define debug(msg)  (static_cast<void>(0))
#else
#define debug(msg)  std::cerr << "Lex: " << msg << std::endl
#endif

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

%%

{WhiteSp}   { /* ignore white spaces */ }
{Comment}   { /* ignore line comments */ }

"\n"        { debug("EOL"); return EOL; }

"var"       { debug("Var"); return VAR; }
"if"        { debug("If"); return IF; }
"goto"      { debug("Goto"); return GOTO; }
"param"     { debug("Param"); return PARAM; }
"call"      { debug("Call"); return CALL; }
"return"    { debug("Return"); return RETURN; }
"end"       { debug("End"); return END; }

{Label}     { debug("Label"); yylval.str_val = std::string(yytext); return LABEL; }
{Function}  { debug("Function"); yylval.str_val = std::string(yytext); return FUNCTION; }
{Symbol}    { debug("Symbol"); yylval.str_val = std::string(yytext); return SYMBOL; }
{Num}       { debug("Num"); yylval.int_val = std::atoi(yytext); return NUM; }
[\[\]:=]    { debug("Other"); return yytext[0]; }
{Op}        { debug("Op"); yylval.op_val = GetOpVal(yytext); return OP; }
{LogicOp}   { debug("LogicOp"); yylval.op_val = GetOpVal(yytext); return LOGICOP; }

%%

// get 'op_val' of the specific operator
TokenOp GetOpVal(std::string_view op) {
  const char *kOpStr[] = {TOKEN_OPERATORS(TOKEN_EXPAND_SECOND)};
  int op_val = 0;
  for (const auto &i : kOpStr) {
    if (op == i) return static_cast<TokenOp>(op_val);
    ++op_val;
  }
  assert(false);
  return -1;
}
