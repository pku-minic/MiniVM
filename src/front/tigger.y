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

// some magic, in order to be compatible with the new version
// and the old version of Bison, since the newer version of
// Bison will put the declaration of 'yyparse' function to
// the generated header, but we can not include VM related
// definitions in the header, because the older version does
// not support '%code requires'.
#define CONT()  (*reinterpret_cast<VMInstContainer *>(cont))

// lexer
int yylex();

// error logger
void yyerror(void *cont, const char *message);

// convert binary 'TokenOp' to 'InstOp'
static InstOp GetBinaryOp(VMInstContainer &cont, TokenOp bin_op);

%}

// enable line number
%locations

// parameter of 'yyparse' function
%parse-param { void *cont }

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
    CONT().LogLineNum(@$.first_line);
    CONT().PushLoad(4);
    CONT().PushArr($1);
    CONT().PushLoad($3);
    CONT().PushLoad($1);
    CONT().PushStore();
  }
  | VARIABLE '=' MALLOC NUM {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLoad($4);
    CONT().PushArr($1);
  }
  ;

FunctionDef
  : FunctionHeader EOL Expressions FunctionEnd
  ;

FunctionHeader
  : FUNCTION '[' NUM ']' '[' NUM ']' {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLabel($1);
    CONT().EnterFunc($3, $6);
  }
  ;

Expressions
  : FullExpression
  | Expressions FullExpression
  ;

FunctionEnd
  : END FUNCTION { CONT().ExitFunc(); }
  ;

FullExpression
  : Expression EOL
  | EOL
  ;

Expression
  : Reg '=' Reg BinOp Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($3);
    CONT().PushLdReg($5);
    CONT().PushOp(GetBinaryOp(CONT(), $4));
    CONT().PushStReg($1);
  }
  | Reg '=' Reg BinOp NUM {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($3);
    CONT().PushLoad($5);
    CONT().PushOp(GetBinaryOp(CONT(), $4));
    CONT().PushStReg($1);
  }
  | Reg '=' OP Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($4);
    auto op = InstOp::Add;
    switch ($3) {
      case TokenOp::Sub: op = InstOp::Neg; break;
      case TokenOp::Not: op = InstOp::LNot; break;
      default: CONT().LogError("invalid unary operator"); break;
    }
    CONT().PushOp(op);
    CONT().PushStReg($1);
  }
  | Reg '=' Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($3);
    CONT().PushStReg($1);
  }
  | Reg '=' NUM {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLoad($3);
    CONT().PushStReg($1);
  }
  | Reg '[' NUM ']' '=' Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($6);
    CONT().PushLdReg($1);
    CONT().PushLoad($3);
    CONT().PushOp(InstOp::Add);
    CONT().PushStore();
  }
  | Reg '=' Reg '[' NUM ']' {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($3);
    CONT().PushLoad($5);
    CONT().PushOp(InstOp::Add);
    CONT().PushLoad();
    CONT().PushStReg($1);
  }
  | IF Reg LOGICOP Reg GOTO LABEL {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($2);
    CONT().PushLdReg($4);
    CONT().PushOp(GetBinaryOp(CONT(), $3));
    CONT().PushBnz($6);
  }
  | GOTO LABEL {
    CONT().LogLineNum(@$.first_line);
    CONT().PushJump($2);
  }
  | LABEL ':' { CONT().PushLabel($1); }
  | CALL FUNCTION {
    CONT().LogLineNum(@$.first_line);
    CONT().PushCall($2);
  }
  | RETURN {
    CONT().LogLineNum(@$.first_line);
    CONT().PushOp(InstOp::Ret);
  }
  | STORE Reg NUM {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdReg($2);
    CONT().PushStFrame($3);
  }
  | LOAD NUM Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdFrame($2);
    CONT().PushStReg($3);
  }
  | LOAD VARIABLE Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLoad($2);
    CONT().PushLoad();
    CONT().PushStReg($3);
  }
  | LOADADDR NUM Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLdFrameAddr($2);
    CONT().PushStReg($3);
  }
  | LOADADDR VARIABLE Reg {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLoad($2);
    CONT().PushStReg($3);
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

void yyerror(void *cont, const char *message) {
  CONT().LogError(message);
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
