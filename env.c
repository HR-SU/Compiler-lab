#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/

E_enventry E_VarEntry(Ty_ty ty)
{
	E_enventry e = checked_malloc(sizeof(*e));
	e->kind = E_varEntry;
	e->u.var.ty = ty;
	return e;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result)
{
	E_enventry e = checked_malloc(sizeof(*e));
	e->kind = E_funEntry;
	e->u.fun.formals = formals;
	e->u.fun.result = result;
	return e;
}

S_table E_base_tenv(void)
{
	S_table tenv = S_empty();
	S_enter(tenv, S_Symbol("int"), Ty_Int());
	S_enter(tenv, S_Symbol("string"), Ty_String());
	return tenv;
}

S_table E_base_venv(void)
{
	S_table venv = S_empty();
	Ty_tyList tylist = Ty_TyList(Ty_String(), NULL);
	S_enter(venv, S_Symbol("print"), E_FunEntry(tylist, NULL));
	S_enter(venv, S_Symbol("flush"), E_FunEntry(NULL, NULL));
	S_enter(venv, S_Symbol("getchar"), E_FunEntry(NULL, Ty_String()));
	S_enter(venv, S_Symbol("ord"), E_FunEntry(tylist, Ty_Int()));
	S_enter(venv, S_Symbol("size"), E_FunEntry(tylist, Ty_Int()));
	Ty_tyList tylist1 = Ty_TyList(Ty_String(), tylist);
	S_enter(venv, S_Symbol("concat"), E_FunEntry(tylist1, Ty_String()));
	Ty_tyList tylist2 = Ty_TyList(Ty_Int(), NULL);
	S_enter(venv, S_Symbol("chr"), E_FunEntry(tylist2, Ty_String()));
	S_enter(venv, S_Symbol("not"), E_FunEntry(tylist2, Ty_Int()));
	S_enter(venv, S_Symbol("exit"), E_FunEntry(tylist2, NULL));
	Ty_tyList tylist3 = Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), tylist2));
	S_enter(venv, S_Symbol("substring"), E_FunEntry(tylist3, Ty_String()));
	return NULL;
}
