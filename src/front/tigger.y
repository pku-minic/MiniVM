/* NOTE:
 *
 *  In order to be compatible with the built-in version of Bison
 *  in macOS, we must avoid using most of the new features that
 *  appeared after Bison 2.3.
 *
 *  Apple sucks!
 *
 *  By MaxXing.
 */

%{

#include <cstdint>

#include "vm/instcont.h"
#include "front/token.h"

// lexer
int yylex();

// error logger
void yyerror(VMInstContainer &cont, const char *message);

// convert binary 'TokenOp' to 'InstOp'
static InstOp GetBinaryOp(VMInstContainer &cont, TokenOp bin_op);

%}

// enable line number
%locations

// parameter of 'yyparse' function
%parse-param { VMInstContainer &cont }

// actual value of token/non-terminals
%union {
  const char *str_val;
  std::int32_t int_val;
  TokenOp op_val;
  TokenReg reg_val;
}

// all tokens
%token EOL IF GOTO CALL RETURN END LOAD STORE LOADADDR MALLOC
%token <str_val> LABEL FUNCTION VARIABLE
%token <int_val> NUM
%token <reg_val> REG
%token <op_val> OP LOGICOP

// type of some non-terminals
%type <op_val> BinOp
%type <int_val> Reg

%%

Program
  : /* Empty */
  | Program GlobalVarDecl EOL
  | Program FunctionDef EOL
  | Program EOL
  ;

GlobalVarDecl
  : VARIABLE '=' NUM {
    cont.LogLineNum(@$.first_line);
    cont.PushVar($1);
    cont.PushLoad($3);
    cont.PushStore($1);
  }
  | VARIABLE '=' MALLOC NUM {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($4);
    cont.PushArr($1);
  }
  ;

FunctionDef
  : FunctionHeader EOL Expressions FunctionEnd
  ;

FunctionHeader
  : FUNCTION '[' NUM ']' '[' NUM ']' {
    cont.LogLineNum(@$.first_line);
    cont.PushLabel($1);
    cont.EnterFunc($3, $6);
  }
  ;

Expressions
  : FullExpression
  | Expressions FullExpression
  ;

FunctionEnd
  : END FUNCTION { cont.ExitFunc(); }
  ;

FullExpression
  : Expression EOL
  | EOL
  ;

Expression
  : Reg '=' Reg BinOp Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($3);
    cont.PushLdReg($5);
    cont.PushOp(GetBinaryOp(cont, $4));
    cont.PushStReg($1);
  }
  | Reg '=' Reg BinOp NUM {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($3);
    cont.PushLoad($5);
    cont.PushOp(GetBinaryOp(cont, $4));
    cont.PushStReg($1);
  }
  | Reg '=' OP Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($4);
    auto op = InstOp::Add;
    switch ($3) {
      case TokenOp::Sub: op = InstOp::Neg; break;
      case TokenOp::Not: op = InstOp::LAnd; break;
      default: cont.LogError("invalid unary operator"); break;
    }
    cont.PushOp(op);
    cont.PushStReg($1);
  }
  | Reg '=' Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($3);
    cont.PushStReg($1);
  }
  | Reg '=' NUM {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($3);
    cont.PushStReg($1);
  }
  | Reg '[' NUM ']' '=' Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($6);
    cont.PushLdReg($1);
    cont.PushLoad($3);
    cont.PushOp(InstOp::Add);
    cont.PushStore();
  }
  | Reg '=' Reg '[' NUM ']' {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($3);
    cont.PushLoad($5);
    cont.PushOp(InstOp::Add);
    cont.PushLoad();
    cont.PushStReg($1);
  }
  | IF Reg LOGICOP Reg GOTO LABEL {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($2);
    cont.PushLdReg($4);
    cont.PushOp(GetBinaryOp(cont, $3));
    cont.PushBnz($6);
  }
  | GOTO LABEL {
    cont.LogLineNum(@$.first_line);
    cont.PushJump($2);
  }
  | LABEL ':' { cont.PushLabel($1); }
  | CALL FUNCTION {
    cont.LogLineNum(@$.first_line);
    cont.PushCall($2);
  }
  | RETURN {
    cont.LogLineNum(@$.first_line);
    cont.PushOp(InstOp::Ret);
  }
  | STORE Reg NUM {
    cont.LogLineNum(@$.first_line);
    cont.PushLdReg($2);
    cont.PushStFrame($3);
  }
  | LOAD NUM Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLdFrame($2);
    cont.PushStReg($3);
  }
  | LOAD VARIABLE Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($2);
    cont.PushStReg($3);
  }
  | LOADADDR NUM Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLdFrameAddr($2);
    cont.PushStReg($3);
  }
  | LOADADDR VARIABLE Reg {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad(0);
    cont.PushLdAddr($2);
    cont.PushStReg($3);
  }
  ;

BinOp
  : OP { $$ = $1; }
  | LOGICOP { $$ = $1; }
  ;

Reg
  : REG { $$ = static_cast<std::int32_t>($1); }
  ;

%%

void yyerror(VMInstContainer &cont, const char *message) {
  cont.LogError(message);
}

static InstOp GetBinaryOp(VMInstContainer &cont, TokenOp bin_op) {
  auto op = InstOp::Add;
  switch (bin_op) {
    case TokenOp::Add: op = InstOp::Add; break;
    case TokenOp::Sub: op = InstOp::Sub; break;
    case TokenOp::Mul: op = InstOp::Mul; break;
    case TokenOp::Div: op = InstOp::Div; break;
    case TokenOp::Mod: op = InstOp::Mod; break;
    case TokenOp::Ne: op = InstOp::Ne; break;
    case TokenOp::Eq: op = InstOp::Eq; break;
    case TokenOp::Gt: op = InstOp::Gt; break;
    case TokenOp::Lt: op = InstOp::Lt; break;
    case TokenOp::Ge: op = InstOp::Ge; break;
    case TokenOp::Le: op = InstOp::Le; break;
    case TokenOp::Or: op = InstOp::LOr; break;
    case TokenOp::And: op = InstOp::LAnd; break;
    default: cont.LogError("invalid binary operator"); break;
  }
  return op;
}
