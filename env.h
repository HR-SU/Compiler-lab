#ifndef __ENV_H_
#define __ENV_H_

#include "types.h"

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
	enum {E_varEntry, E_funEntry, E_loopEntry} kind;
	union 
	{
		struct {Ty_ty ty;} var;
		struct {Ty_tyList formals; Ty_ty result;} fun;
                struct {Ty_ty ty;} loop;
	} u;
};

E_enventry E_VarEntry(Ty_ty ty);
E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result);
E_enventry E_LoopEntry(Ty_ty ty);

S_table E_base_tenv(void);
S_table E_base_venv(void);

#endif
