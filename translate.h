#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "util.h"
#include "absyn.h"
#include "temp.h"
#include "frame.h"

/* Lab5: your code below */

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_expList_ *Tr_expList;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

Tr_exp Tr_void();
Tr_exp Tr_simpleVar(Tr_access access, Tr_level level);
Tr_exp Tr_fieldVar(Tr_exp exp, int num);
Tr_exp Tr_subscriptVar(Tr_exp exp, Tr_exp offset);
Tr_exp Tr_intExp(int val);
Tr_exp Tr_stringExp(string str);
Tr_exp Tr_callExp(Tr_level crtLevel, Tr_level level, Temp_label label, Tr_expList args);
Tr_exp Tr_opExp(Tr_exp left, Tr_exp right, A_oper op);
Tr_exp Tr_bicmpExp(Tr_exp left, Tr_exp right, A_oper op);
Tr_exp Tr_recordExp(Tr_expList expList, int size);
Tr_exp Tr_seqExp(Tr_exp first, Tr_exp second);
Tr_exp Tr_assignExp(Tr_exp var, Tr_exp val);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label done);
Tr_exp Tr_breakExp(Temp_label done);
Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done);
Tr_exp Tr_letExp(Tr_expList decs, Tr_exp body);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);
Tr_exp Tr_varDec(Tr_access access, Tr_exp init);

void Tr_procEntryExit(Tr_exp body, Tr_level level);

F_fragList Tr_getResult(void);
#endif
