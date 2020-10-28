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
%token EOL VAR IF GOTO PARAM CALL RETURN ADDROFOP END
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
    cont.LogLineNum(@$.first_line);
    cont.PushVar($2);
  }
  | VAR NUM SYMBOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($2);
    cont.PushArr($3);
  }
  ;

FunctionDef
  : FunctionHeader EOL Statements FunctionEnd
  ;

FunctionHeader
  : FUNCTION '[' NUM ']' {
    cont.LogLineNum(@$.first_line);
    cont.PushLabel($1);
    cont.EnterFunc($3);
  }
  ;

Statements
  : Statement
  | Statements Statement
  ;

FunctionEnd
  : END FUNCTION { cont.ExitFunc(); }
  ;

Statement
  : Expression EOL
  | Declaration EOL
  | EOL
  ;

Expression
  : SYMBOL '=' RightValue BinOp RightValue {
    cont.LogLineNum(@$.first_line);
    $3.GenerateLoad(cont);
    $5.GenerateLoad(cont);
    cont.PushOp(GetBinaryOp(cont, $4));
    cont.PushStore($1);
  }
  | SYMBOL '=' OP RightValue {
    cont.LogLineNum(@$.first_line);
    $4.GenerateLoad(cont);
    auto op = InstOp::Add;
    switch ($3) {
      case TokenOp::Sub: op = InstOp::Neg; break;
      case TokenOp::Not: op = InstOp::LAnd; break;
      default: cont.LogError("invalid unary operator"); break;
    }
    cont.PushOp(op);
    cont.PushStore($1);
  }
  | SYMBOL '=' ADDROFOP SYMBOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad(0);
    cont.PushLdAddr($4);
    cont.PushStore($1);
  }
  | SYMBOL '=' RightValue {
    cont.LogLineNum(@$.first_line);
    $3.GenerateLoad(cont);
    cont.PushStore($1);
  }
  | SYMBOL '[' RightValue ']' '=' RightValue {
    cont.LogLineNum(@$.first_line);
    $6.GenerateLoad(cont);
    $3.GenerateLoad(cont);
    cont.PushLdAddr($1);
    cont.PushStore();
  }
  | SYMBOL '=' SYMBOL '[' RightValue ']' {
    cont.LogLineNum(@$.first_line);
    $5.GenerateLoad(cont);
    cont.PushLdAddr($3);
    cont.PushLoad();
    cont.PushStore($1);
  }
  | IF RightValue LOGICOP RightValue GOTO LABEL {
    cont.LogLineNum(@$.first_line);
    $2.GenerateLoad(cont);
    $4.GenerateLoad(cont);
    cont.PushOp(GetBinaryOp(cont, $3));
    cont.PushBnz($6);
  }
  | GOTO LABEL {
    cont.LogLineNum(@$.first_line);
    cont.PushJump($2);
  }
  | LABEL ':' { cont.PushLabel($1); }
  | PARAM RightValue {
    cont.LogLineNum(@$.first_line);
    $2.GenerateLoad(cont);
  }
  | CALL FUNCTION {
    cont.LogLineNum(@$.first_line);
    cont.PushCall($2);
  }
  | SYMBOL '=' CALL FUNCTION {
    cont.LogLineNum(@$.first_line);
    cont.PushCall($4);
    cont.PushStore($1);
  }
  | RETURN RightValue {
    cont.LogLineNum(@$.first_line);
    $2.GenerateLoad(cont);
    cont.PushOp(InstOp::Ret);
  }
  | RETURN {
    cont.LogLineNum(@$.first_line);
    cont.PushOp(InstOp::Ret);
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
