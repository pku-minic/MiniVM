%{

#include <string>
#include <iostream>

#include "xstl/style.h"
#include "front/token.h"
#include "vm/instcont.h"

// convert binary 'TokenOp' to 'InstOp'
InstOp GetBinaryOp(VMInstContainer &cont, TokenOp bin_op);

%}

// parameter of 'yyparse' function
%parse-param { VMInstContainer &cont }

// actual value of token
%union {
  std::string str_val;
  int         int_val;
  TokenOp     op_val;
}

// all tokens
%token EOF 0
%token EOL VAR IF GOTO PARAM CALL RETURN END
%token <str_val> LABEL FUNCTION SYMBOL
%token <int_val> NUM
%token <op_val> OP LOGICOP

// type of some non-terminals
%type <int_val> RightValue BinOp

%%

Program
  : /* Empty */
  | Program Declaration
  | Program FunctionDecl
  | Program EOF { cont.SealContainer(); }
  ;

FunctionDecl
  : FunctionName Statements FunctionEnd
  ;

FunctionName
  : FUNCTION '[' NUM ']' EOL {
    cont.LogLineNum(@$.first_line);
    cont.EnterFunc($3);
    cont.PushLabel($1);
  }
  ;

FunctionEnd
  : END FUNCTION EOL { cont.ExitFunc(); }
  ;

Declaration
  : VAR SYMBOL EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushVar($2);
  }
  | VAR NUM SYMBOL EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($2);
    cont.PushArr($3);
  }
  ;

Statements
  : Expression
  | Declaration
  | Statements Expression
  | Statements Declaration
  ;

RightValue
  : SYMBOL { $$ = $1; }
  | NUM { $$ = $1; }
  ;

BinOp
  : OP { $$ = $1; }
  | LOGICOP { $$ = $1; }
  ;

Expression
  : SYMBOL '=' RightValue BinOp RightValue EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($3);
    cont.PushLoad($5);
    cont.PushOp(GetBinaryOp(cont, $4));
    cont.PushStore($1);
  }
  | SYMBOL '=' OP RightValue EOL {
    cont.LogLineNum(@$.first_line);
    if ($3 == TokenOp::Addr) {
      cont.PushLoad(0);
      cont.PushLdAddr($4);
    }
    else {
      cont.PushLoad($4);
      InstOp op;
      switch ($3) {
        case TokenOp::Sub: op = InstOp::Neg; break;
        case TokenOp::Not: op = InstOp::LAnd; break;
        default: cont.LogError("invalid unary operator"); break;
      }
      cont.PushOp(op);
    }
    cont.PushStore($1);
  }
  | SYMBOL '=' RightValue EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($3);
    cont.PushStore($1);
  }
  | SYMBOL '[' RightValue ']' '=' RightValue EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($6);
    cont.PushLoad($3);
    cont.PushLdAddr($1);
    cont.PushStore();
  }
  | SYMBOL '=' SYMBOL '[' RightValue ']' EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($5);
    cont.PushLdAddr($3);
    cont.PushLoad();
    cont.PushStore($1);
  }
  | IF RightValue LOGICOP RightValue GOTO LABEL EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($2);
    cont.PushLoad($4);
    cont.PushOp(GetBinaryOp(cont, $3));
    cont.PushBnz($6);
  }
  | GOTO LABEL EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushJump($2);
  }
  | LABEL ':' EOL { cont.PushLabel($1); }
  | PARAM RightValue EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($2);
    cont.PushOp(InstOp::Param);
  }
  | CALL FUNCTION EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushCall($2);
  }
  | SYMBOL '=' CALL FUNCTION EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushCall($4);
    cont.PushStore($1);
  }
  | RETURN RightValue EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushLoad($2);
    cont.PushOp(InstOp::Ret);
  }
  | RETURN EOL {
    cont.LogLineNum(@$.first_line);
    cont.PushOp(InstOp::Ret);
  }
  | EOL
  ;

%%

void yyerror(const char *message) {
  using namespace xstl;
  std::cerr << style("Br") << "error: " << message << std::endl;
}

InstOp GetBinaryOp(VMInstContainer &cont, TokenOp bin_op) {
  InstOp op;
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
