#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/


typedef void* Tr_exp;
struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}

struct expty transVar(S_table venv, S_table tenv, A_var v)
{
	switch(v->kind) {
		case A_simpleVar: {
			E_enventry var = S_look(venv, v->u.simple);
			if(var && var->kind == E_varEntry) {
				return expTy(NULL, var->u.var.ty);
			}
			else {
				EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
			}
		}
		case A_fieldVar: {
			struct expty var = transVar(venv, tenv, v->u.field.var);
			if(var.ty->kind == Ty_record) {
				string name = S_name(v->u.field.sym);
				for(Ty_fieldList fields = var.ty->u.record; fields;
				fields = fields->tail) {
					if(name == S_name(fields->head->name)) {
						return expTy(NULL, fields->head->ty);
					}
				}
				EM_error(v->pos, "can't find field name");
				return expTy(NULL, Ty_Nil());
			}
			else {
				EM_error(v->pos, "undefined record variable");
				return expTy(NULL, Ty_Nil());
			}
		}
		case A_subscriptVar: {
			struct expty var = transVar(venv, tenv, v->u.subscript.var);
			if(var.ty->kind == Ty_array) {
				struct expty exp = transExp(venv, tenv, v->u.subscript.exp);
				if(exp.ty->kind != Ty_int) {
					EM_error(v->u.subscript.exp->pos, "integer required");
				}
				return expTy(NULL, var.ty->u.array);
			}
			else {
				EM_error(v->pos, "undefined array variable");
				return expTy(NULL, Ty_Nil());
			}
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a)
{
	switch(a->kind) {
		case A_varExp: {
			struct expty var = transVar(venv, tenv, a->u.var);
			return expTy(NULL, var.ty);
		}
		case A_nilExp: {
			return expTy(NULL, Ty_Void());
		}
		case A_intExp: {
			return expTy(NULL, Ty_Int());
		}
		case A_stringExp: {
			return expTy(NULL, Ty_String());
		}
		case A_callExp: {
			E_enventry func = S_look(venv, a->u.call.func);
			if(func && func->kind == E_funEntry) {
				A_expList args = a->u.call.args;
				for(Ty_tyList formal = func->u.fun.formals; formal; formal = formal->tail) {
					if(args == NULL) {
						EM_error(a->pos, "args number don't match");
						return expTy(NULL, func->u.fun.result);
					}
					A_exp arg = args->head;
					struct expty argty = transExp(venv, tenv, arg);
					if(argty.ty != formal) {
						EM_error(arg->pos, "args type don't match");
					}
					args = args->tail;
					return expTy(NULL, func->u.fun.result);
				}
			}
			else {
				EM_error(a->pos, "undefined function %s", a->u.call.func);
				return expTy(NULL, Ty_Int());
			}
		}
		case A_opExp: {
			struct expty left = transExp(venv, tenv, a->u.op.left);
			struct expty right = transExp(venv, tenv, a->u.op.right);
			switch(a->u.op.oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp: {
					if(left.ty->kind != Ty_int) {
						EM_error(a->u.op.left->pos, "integer required");
					}
					if(right.ty->kind != Ty_int) {
						EM_error(a->u.op.right->pos, "integer required");
					}
					return expTy(NULL, Ty_Int());
				}
				case A_ltOp:
				case A_leOp:
				case A_gtOp:
				case A_geOp: {
					if(left.ty->kind != Ty_int && left.ty->kind != Ty_string) {
						EM_error(a->u.op.left->pos, "integer or string required");
					}
					if(right.ty->kind != Ty_int&& right.ty->kind != Ty_string) {
						EM_error(a->u.op.right->pos, "integer or string required");
					}
					if(left.ty->kind != right.ty->kind) {
						EM_error(a->u.op.right->pos, "same type required");
					}
					return expTy(NULL, Ty_Int());
				}
				case A_eqOp:
				case A_neqOp: {
					if(left.ty->kind != Ty_int && left.ty->kind != Ty_string &&
					left.ty->kind != Ty_record && left.ty->kind != Ty_array) {
						EM_error(a->u.op.left->pos, "integer or string or\
						\record or array required");
					}
					if(right.ty->kind != Ty_int&& right.ty->kind != Ty_string &&
					right.ty->kind != Ty_record && right.ty->kind != Ty_array) {
						EM_error(a->u.op.right->pos, "integer or string or\
						\record or array required");
					}
					if(left.ty->kind != right.ty->kind) {
						EM_error(a->u.op.right->pos, "same type required");
					}
					return expTy(NULL, Ty_Int());
				}
				default: {
					EM_error(a->pos, "undefined operator");
					return expTy(NULL, Ty_Int());
				}
			}
		}
		case A_recordExp: {
			Ty_ty recordType = S_look(tenv, a->u.record.typ);
			if(recordType && recordType->kind == Ty_record) {
				for(A_efieldList fields = a->u.record.fields; fields; 
				fields = fields->tail) {
					string name = S_name(fields->head->name);
					bool found = FALSE;
					for(Ty_fieldList f = recordType->u.record; f; f = f->tail) {
						if(name == S_name(f->head->name)) {
							found = TRUE;
							struct expty exp = transExp(venv, tenv, fields->head->exp);
							if(exp.ty->kind != f->head->ty->kind) {
								EM_error(fields->head->exp->pos, "field type don't match");
							}
							break;
						}
					}
					if(!found) {
						EM_error(fields->head->exp->pos, "undefined field name");
					}
				}
				return expTy(NULL, Ty_Record(recordType->u.record));
			}
			else {
				EM_error(a->pos, "undefined record type");
				return expTy(NULL, Ty_Record(NULL));
			}
		}
		case A_seqExp: {
			for(A_expList expList = a->u.seq; expList; expList = expList->tail) {
				struct expty exp = transExp(venv, tenv, expList->head);
				if(expList->tail == NULL) {
					return expTy(NULL, exp.ty);
				}
			}
		}
		case A_assignExp: {
			struct expty var = transVar(venv, tenv, a->u.assign.var);
			struct expty exp = transExp(venv, tenv, a->u.assign.exp);
			if(var.ty != exp.ty) {
				EM_error(a->pos, "assign type don't match");
			}
			return expTy(NULL, Ty_Nil());
		}
		case A_ifExp: {
			struct expty test = transExp(venv, tenv, a->u.iff.test);
			if(test.ty->kind != Ty_int) {
				EM_error(a->u.iff.test->pos, "integer required");
			}
			struct expty then = transExp(venv, tenv, a->u.iff.then);
			struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
			if(then.ty->kind != elsee.ty->kind) {
				EM_error(a->u.iff.then->pos, "then clause and else clause\
				\should have the same type");
			}
			return expTy(NULL, then.ty);
		}
		case A_whileExp: {
			struct expty test = transExp(venv, tenv, a->u.whilee.test);
			if(test.ty->kind != Ty_int) {
				EM_error(a->u.whilee.test->pos, "integer required");
			}
			struct expty body = transExp(venv, tenv, a->u.whilee.body);
			if(body.ty->kind != Ty_nil) {
				EM_error(a->u.whilee.body->pos, "nil required");
			}
			return expTy(NULL, Ty_Nil());
		}
		case A_forExp: {
			struct expty lo = transExp(venv, tenv, a->u.forr.lo);
			if(lo.ty->kind != Ty_int) {
				EM_error(a->u.forr.lo->pos, "integer required");
			}
			struct expty hi = transExp(venv, tenv, a->u.forr.hi);
			if(hi.ty->kind != Ty_int) {
				EM_error(a->u.forr.hi->pos, "integer required");
			}
			struct expty body = transExp(venv, tenv, a->u.forr.body);
			if(body.ty->kind != Ty_nil) {
				EM_error(a->u.forr.body->pos, "nil required");
			}
			return expTy(NULL, Ty_Nil());
		}
		case A_breakExp: {
			return expTy(NULL, Ty_Nil());
		}
		case A_letExp: {
			struct expty exp;
			A_decList dec;
			S_beginScope(venv);
			S_beginScope(tenv);
			for(dec = a->u.let.decs; dec; dec = dec->tail) {
				transDec(venv, tenv, dec->head);
			}
			exp = transExp(venv, tenv, a->u.let.body);
			S_endScope(tenv);
			S_endScope(venv);
			return exp;
		}
		case A_arrayExp: {
			Ty_ty arrayType = S_look(tenv, a->u.array.typ);
			if(arrayType && arrayType->kind == Ty_array) {
				struct expty size = transExp(venv, tenv, a->u.array.size);
				if(size.ty->kind != Ty_int) {
					EM_error(a->u.array.size->pos, "integer required");
				}
				struct expty init = transExp(venv, tenv, a->u.array.init);
				if(init.ty->kind != arrayType->u.array) {
					EM_error(a->u.array.init->pos, "array init type don't match");
				}
				return expTy(NULL, Ty_Array(arrayType->u.array));
			}
			else {
				EM_error(a->pos, "undefined aray type");
				return expTy(NULL, Ty_Nil());
			}
		}
	}
}

void transDec(S_table venv, S_table tenv, A_dec d)
{
	switch(d->kind) {
		case A_varDec: {
			struct expty init = transExp(venv, tenv, d->u.var.init);
			if(d->u.var.typ != NULL) {
				Ty_ty ty = S_look(tenv, d->u.var.typ);
				if(ty->kind == Ty_nil) {
					EM_error(d->pos, "cannot declare a var with type nil");
				}
				if(init.ty->kind != ty->kind) {
					EM_error(d->pos, "declare type don't match");
				}
				S_enter(venv, d->u.var.var, E_VarEntry(ty));
			}
			else {
				S_enter(venv, d->u.var.var, E_VarEntry(init.ty));
			}
			break;
		}
		case A_typeDec: {
			for(A_nametyList tylist = d->u.type; tylist; tylist = tylist->tail) {
				Ty_ty ty = transTy(tenv, tylist->head->ty);
				if(ty == NULL) {
					EM_error(tylist->head->ty->pos, "undefined type name");
				}
				else {
					S_enter(tenv, tylist->head->name, ty);
				}
			}
			break;
		}
		case A_functionDec: {
			for(A_fundecList funList = d->u.function; funList; 
			funList = funList->tail) {
				A_fundec f = funList->head;
				Ty_ty resultTy = S_look(tenv, f->result);
				if(resultTy == NULL) {
					EM_error(f->pos, "undefined result type");
				}
				Ty_tyList formalTys = maketylist(tenv, f->params);
				S_enter(venv, f->name, E_FunEntry(formalTys, resultTy));
				S_beginScope(venv);
				A_fieldList params = f->params;
				Ty_tyList paramTys= formalTys;
				for(; params; params = params->tail, paramTys = paramTys->tail) {
					S_enter(venv, params->head->name, E_VarEntry(paramTys->head));
				}
				struct expty body = transExp(venv, tenv, f->body);
				if(body.ty->kind != resultTy->kind) {
					EM_error(f->body->pos, "return type don't match");
				}
				S_endScope(venv);
			}
			break;
		}
		default: {
			EM_error(d->pos, "error");
		}
	}
	return;
}

Ty_ty transTy (S_table tenv, A_ty a)
{
	switch(a->kind) {
		case A_nameTy: {
			return S_look(tenv, a->u.name);
		}
		case A_arrayTy: {
			Ty_ty type = S_look(tenv, a->u.array);
			if(type == NULL) {
				EM_error(a->pos, "undefined type name");
				return Ty_Array(Ty_Nil());
			}
			return Ty_Array(type);
		}
		case A_recordTy: {
			Ty_tyList fields = maketylist(tenv, a->u.record);
			return Ty_Record(fields);
		}
	}
}

Ty_tyList maketylist(S_table tenv, A_fieldList flist) {
	Ty_tyList tylist = NULL;
	for(A_fieldList p = flist; p; p = p->tail) {
		Ty_ty ty = S_look(tenv, p->head->typ);
		if(ty == NULL) {
			EM_error(p->head->pos, "undefined param type");
			ty = Ty_Nil();
		}
		tylist = Ty_TyList(ty, tylist);
	}
	return tylist;
}

void SEM_transProg(A_exp exp)
{
	S_table venv = E_base_venv();
	S_table tenv = E_base_tenv();
	transExp(venv, tenv, exp);
}