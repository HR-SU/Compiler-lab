/* Lab3: You are free to modify this file (**ONLY** modify this file). 
 * But for your convinience, 
 * please read absyn.h carefully first.
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h" 
#include "errormsg.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
 exit(1);
}
%}

/* the fields in YYTYPE */
%union {
	int pos;
	int ival;
	string sval;
	A_var var;
	A_exp exp;
	A_expList expList;
	A_dec dec;
	A_decList decList;
	A_fundec fundec;
	A_fundecList fundecList;
	A_namety namety;
	A_nametyList nametyList;
	A_field field;
	A_fieldList fieldList;
	A_efield efield;
	A_efieldList efieldList;
	A_ty ty;
}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%left OR
%left AND
%nonassoc EQ NEQ LT GT LE GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%type <exp> exp program voidexp seqexp
%type <var> lvalue
%type <expList> args exps
%type <ty> ty
%type <dec> dec vardec 
%type <fundec> fundec
%type <fundecList> fundecs
%type <namety> tydec
%type <nametyList> tydecs
%type <decList> decs
%type <field> tyfield
%type <fieldList> tyfields
%type <efield> recorditem 
%type <efieldList> recorditems

/* Lab3: One solution: you can fill the following rules directly.
 * Of course, you can modify (add, delete or change) any rules.
 */
%start program

%%

program: exp  {absyn_root=$1;}

exp: ID
   | exp AND exp {$$=A_IfExp(EM_tokPos,$1,$3,A_IntExp(EM_tokPos,0));}
   | exp OR exp {$$=A_IfExp(EM_tokPos,$1,A_IntExp(EM_tokPos,1),$3);}
   | NIL {$$=A_NilExp(EM_tokPos);}
   | INT {$$=A_IntExp(EM_tokPos,$1);}
   | STRING {$$=A_StringExp(EM_tokPos,$1);}
   | MINUS INT {$$=A_IntExp(EM_tokPos,-$1);}
   | ID LPAREN RPAREN {$$=A_CallExp(EM_tokPos,S_Symbol($1),NULL);}
   | ID LPAREN args RPAREN {$$=A_CallExp(EM_tokPos,S_Symbol($1),$3);}
   | exp PLUS exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp MINUS exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp TIMES exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp DIVIDE exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp LT exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp LE exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp GT exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp GE exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp NEQ exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | exp EQ exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
   | ID LBRACE RBRACE {$$=A_RecordExp(EM_tokPos,S_Symbol($1),NULL);}
   | ID LBRACE recorditems RBRACE {$$=A_RecordExp(EM_tokPos,S_Symbol($1),$3);}
   | ID LBRACK exp RBRACK OF exp {$$=A_ArrayExp(EM_tokPos,S_Symbol($1),$3,$6);}
   | lvalue ASSIGN exp {$$=A_AssignExp(EM_tokPos,$1,$3);}
   | IF exp THEN exp ELSE exp {$$=A_IfExp(EM_tokPos,$2,$4,$6);}
   | IF exp THEN exp {$$=A_IfExp(EM_tokPos,$2,$4,NULL);}
   | WHILE exp DO exp {$$=A_WhileExp(EM_tokPos,$2,$4);}
   | FOR ID ASSIGN exp TO exp DO exp {$$=A_ForExp(EM_tokPos,S_Symbol($2),$4,$6,$8);}
   | BREAK {$$=A_BreakExp(EM_tokPos);}
   | LET decs IN seqexp END {$$=A_LetExp(EM_tokPos,$2,$4);}
   | LPAREN exp RPAREN {$$=2;}

exps: exp {$$=A_ExpList($1,NULL);}
    | exps SEMICOLON exp {$$=A_ExpList($3,$1);}

seqexp: exps {$$=A_SeqExp(EM_tokPos,$1);}
	  
recorditem: ID ASSIGN exp {$$=A_Efield(S_Symbol($1),$3);}

recorditems: recorditem {$$=A_EfieldList($1,NULL);}
           | recorditems COMMA recorditem {$$=A_EfieldList($3,$1);}

voidexp: 
	   
args: exp {$$=A_ExpList($1,NULL);}
    | args exp {$$=A_ExpList($2,$1);}

tyfield: ID COLON ID {$$=A_Field(EM_tokPos,S_Symbol($1),S_Symbol($3));}
	
tyfields: tyfield {$$=A_FieldList(EM_tokPos,$1,NULL);}
        | tyfields COMMA tyfield {$$=A_FieldList(EM_tokPos,$3,$1)}
	
ty: ID {$$=A_NameTy(EM_tokPos,S_Symbol($1));}
  | LBRACE tyfields RBRACE {$$=A_RecordTy(EM_tokPos,$2);}
  | ARRAY OF ID {$$=A_ArrayTy(EM_tokPos,S_Symbol($3));}
 
tydec: TYPE ID ASSIGN ty {$$=A_Namety(S_Symbol($2),$4);}

tydecs: tydec {$$=A_NametyList($1,NULL);}
      | tydecs tydec {$$=A_NametyList($2,$1);}

vardec: VAR ID ASSIGN exp {$$=A_VarDec(EM_tokPos,S_Symbol($2),NULL,$4);}
      | VAR ID ID ASSIGN exp {$$=A_VarDec(EM_tokPos,S_Symbol($2),S_Symbol($3),$5);}

fundec: FUNCTION ID LPAREN tyfields RPAREN ASSIGN exp {$$=A_Fundec(EM_tokPos,S_Symbol($2),$4,NULL,$7);}
      | FUNCTION ID LPAREN tyfields RPAREN COLON ID ASSIGN exp {$$=A_Fundec(EM_tokPos,S_Symbol($2),$4,S_Symbol($7),$9);}
	
fundecs: fundec {$$=A_FundecList($1,NULL);}
       | fundecs fundec {$$=A_FundecList($2,$1);}

dec: tydecs {$$=A_TypeDec(EM_tokPos,$1);}
   | vardec {$$=$1;}
   | fundecs {$$=A_FunctionDec(EM_tokPos,$1);}

decs: dec {$$=A_DecList($1,NULL);}
    | decs dec {$$=A_DecList($2,$1);}

lvalue: ID {$$=A_SimpleVar(EM_tokPos,S_Symbol($1));}
      | lvalue DOT ID {$$=A_FieldVar(EM_tokPos,$1,S_Symbol($3));}
	  | lvalue LBRACK exp RBRACK {$$=A_SubscriptVar(EM_tokPos,$1,$3);}
