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
  // definition for non-terminal 'RightValue'
  struct {
    // accept a symbol
    void Accept(const char *sym) {
      val.sym = sym;
      is_sym = true;
    }

    // accept a number
    void Accept(std::int32_t num) {
      val.num = num;
      is_sym = false;
    }

    // generate load instruction
    template <typename InstCont>
    void GenerateLoad(InstCont &cont) const {
      if (is_sym) {
        cont.PushLoad(val.sym);
      }
      else {
        cont.PushLoad(val.num);
      }
    }

    union {
      const char *sym;
      std::int32_t num;
    } val;
    bool is_sym;
  } right_val;
}

// all tokens
%token EOL VAR IF GOTO PARAM CALL RETURN END
%token <str_val> LABEL FUNCTION SYMBOL
%token <int_val> NUM
%token <op_val> OP LOGICOP

// type of some non-terminals
%type <right_val> RightValue
%type <op_val> BinOp

%%

Program
  : /* Empty */
  | Program Declaration EOL
  | Program FunctionDef EOL
  | Program EOL
  ;

Declaration
  : VAR SYMBOL {
    CONT().LogLineNum(@$.first_line);
    CONT().PushVar($2);
  }
  | VAR NUM SYMBOL {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLoad($2);
    CONT().PushArr($3);
  }
  ;

FunctionDef
  : FunctionHeader EOL Statements FunctionEnd
  ;

FunctionHeader
  : FUNCTION '[' NUM ']' {
    CONT().LogLineNum(@$.first_line);
    CONT().PushLabel($1);
    CONT().EnterFunc($3);
  }
  ;

Statements
  : Statement
  | Statements Statement
  ;

FunctionEnd
  : END FUNCTION { CONT().ExitFunc(); }
  ;

Statement
  : Expression EOL
  | Declaration EOL
  | EOL
  ;

Expression
  : SYMBOL '=' RightValue BinOp RightValue {
    CONT().LogLineNum(@$.first_line);
    $3.GenerateLoad(CONT());
    $5.GenerateLoad(CONT());
    CONT().PushOp(GetBinaryOp(CONT(), $4));
    CONT().PushStore($1);
  }
  | SYMBOL '=' OP RightValue {
    CONT().LogLineNum(@$.first_line);
    $4.GenerateLoad(CONT());
    auto op = InstOp::Add;
    switch ($3) {
      case TokenOp::Sub: op = InstOp::Neg; break;
      case TokenOp::Not: op = InstOp::LAnd; break;
      default: CONT().LogError("invalid unary operator"); break;
    }
    CONT().PushOp(op);
    CONT().PushStore($1);
  }
  | SYMBOL '=' RightValue {
    CONT().LogLineNum(@$.first_line);
    $3.GenerateLoad(CONT());
    CONT().PushStore($1);
  }
  | SYMBOL '[' RightValue ']' '=' RightValue {
    CONT().LogLineNum(@$.first_line);
    $6.GenerateLoad(CONT());
    $3.GenerateLoad(CONT());
    CONT().PushLoad($1);
    CONT().PushOp(InstOp::Add);
    CONT().PushStore();
  }
  | SYMBOL '=' SYMBOL '[' RightValue ']' {
    CONT().LogLineNum(@$.first_line);
    $5.GenerateLoad(CONT());
    CONT().PushLoad($3);
    CONT().PushOp(InstOp::Add);
    CONT().PushLoad();
    CONT().PushStore($1);
  }
  | IF RightValue LOGICOP RightValue GOTO LABEL {
    CONT().LogLineNum(@$.first_line);
    $2.GenerateLoad(CONT());
    $4.GenerateLoad(CONT());
    CONT().PushOp(GetBinaryOp(CONT(), $3));
    CONT().PushBnz($6);
  }
  | GOTO LABEL {
    CONT().LogLineNum(@$.first_line);
    CONT().PushJump($2);
  }
  | LABEL ':' { CONT().PushLabel($1); }
  | PARAM RightValue {
    CONT().LogLineNum(@$.first_line);
    $2.GenerateLoad(CONT());
  }
  | CALL FUNCTION {
    CONT().LogLineNum(@$.first_line);
    CONT().PushCall($2);
  }
  | SYMBOL '=' CALL FUNCTION {
    CONT().LogLineNum(@$.first_line);
    CONT().PushCall($4);
    CONT().PushStore($1);
  }
  | RETURN RightValue {
    CONT().LogLineNum(@$.first_line);
    $2.GenerateLoad(CONT());
    CONT().PushOp(InstOp::Ret);
  }
  | RETURN {
    CONT().LogLineNum(@$.first_line);
    CONT().PushOp(InstOp::Ret);
  }
  ;

RightValue
  : SYMBOL { $$.Accept($1); }
  | NUM { $$.Accept($1); }
  ;

BinOp
  : OP { $$ = $1; }
  | LOGICOP { $$ = $1; }
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
