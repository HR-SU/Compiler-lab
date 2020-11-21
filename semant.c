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

Ty_tyList maketylist(S_table tenv, A_fieldList flist);

Ty_ty actual_ty(Ty_ty ty);

struct expty transVar(S_table venv, S_table tenv, A_var v)
{
	switch(v->kind) {
		case A_simpleVar: {
			E_enventry var = S_look(venv, v->u.simple);
			if(var && var->kind == E_varEntry) {
				return expTy(NULL, actual_ty(var->u.var.ty));
			}
            if(var && var->kind == E_loopEntry) {
                return expTy(NULL, actual_ty(var->u.loop.ty));
            }
			EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
            return expTy(NULL, Ty_Void());
		}
		case A_fieldVar: {
			struct expty var = transVar(venv, tenv, v->u.field.var);
			if(var.ty->kind == Ty_record) {
				string name = S_name(v->u.field.sym);
				for(Ty_fieldList fields = var.ty->u.record; fields;
				fields = fields->tail) {
					if(strcmp(name, S_name(fields->head->name)) == 0) {
						return expTy(NULL, actual_ty(fields->head->ty));
					}
				}
				EM_error(v->pos, "field %s doesn't exist", name);
				return expTy(NULL, Ty_Void());
			}
			else {
				EM_error(v->pos, "variable type is not a record");
				return expTy(NULL, Ty_Void());
			}
		}
		case A_subscriptVar: {
			struct expty var = transVar(venv, tenv, v->u.subscript.var);
			if(var.ty->kind == Ty_array) {
				struct expty exp = transExp(venv, tenv, v->u.subscript.exp);
				if(exp.ty->kind != Ty_int) {
					EM_error(v->u.subscript.exp->pos, "integer required");
				}
				return expTy(NULL, actual_ty(var.ty->u.array));
			}
			else {
				EM_error(v->pos, "variable type is not an array");
				return expTy(NULL, Ty_Void());
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
			return expTy(NULL, Ty_Nil());
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
						return expTy(NULL, actual_ty(func->u.fun.result));
					}
					A_exp arg = args->head;
					struct expty argty = transExp(venv, tenv, arg);
					if(argty.ty->kind != formal->head->kind) {
						EM_error(arg->pos, "args type don't match");
					}
					args = args->tail;
				}
				return expTy(NULL, actual_ty(func->u.fun.result));
			}
			else {
				EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
				return expTy(NULL, Ty_Void());
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
						if(strcmp(name, S_name(f->head->name)) == 0) {
							found = TRUE;
							struct expty exp = transExp(venv, tenv, fields->head->exp);
							if(exp.ty->kind != actual_ty(f->head->ty)->kind) {
								EM_error(fields->head->exp->pos, "field type don't match");
							}
							break;
						}
					}
					if(!found) {
						EM_error(fields->head->exp->pos, "undefined field name");
					}
				}
				return expTy(NULL, Ty_Record(actual_ty(recordType->u.record)));
			}
			else {
				EM_error(a->pos, "undefined record type");
				return expTy(NULL, Ty_Nil());
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
			if(a->u.assign.var->kind == A_simpleVar) {
				E_enventry v = S_look(venv, a->u.assign.var->u.simple);
				if(v->kind == E_loopEntry) {
					EM_error(a->u.assign.var->pos, "loop variable can't be assigned");
				}
			}
			struct expty exp = transExp(venv, tenv, a->u.assign.exp);
			if(var.ty != exp.ty) {
				EM_error(a->pos, "assign type don't match");
			}
			return expTy(NULL, Ty_Void());
		}
		case A_ifExp: {
			struct expty test = transExp(venv, tenv, a->u.iff.test);
			if(test.ty->kind != Ty_int) {
				EM_error(a->u.iff.test->pos, "integer required");
			}
			struct expty then = transExp(venv, tenv, a->u.iff.then);
			struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
			if(elsee.ty->kind == Ty_nil && then.ty->kind != Ty_void) {
				EM_error(a->u.iff.then->pos, "if-then exp's body must produce no value");
				return expTy(NULL, Ty_Void());
			}
			if(actual_ty(then.ty)->kind != actual_ty(elsee.ty)->kind) {
				printf("%d", elsee.ty->kind);
				EM_error(a->u.iff.then->pos, "then clause and else clause should have the same type");
			}
			return expTy(NULL, actual_ty(then.ty));
		}
		case A_whileExp: {
			struct expty test = transExp(venv, tenv, a->u.whilee.test);
			if(test.ty->kind != Ty_int) {
				EM_error(a->u.whilee.test->pos, "integer required");
			}
			struct expty body = transExp(venv, tenv, a->u.whilee.body);
			if(body.ty->kind != Ty_void) {
				EM_error(a->u.whilee.body->pos, "while body must produce no value");
			}
			return expTy(NULL, Ty_Void());
		}
		case A_forExp: {
            S_beginScope(venv);
            S_enter(venv, a->u.forr.var, E_LoopEntry(Ty_Int()));
			struct expty lo = transExp(venv, tenv, a->u.forr.lo);
			if(lo.ty->kind != Ty_int) {
				EM_error(a->u.forr.lo->pos, "for exp's range type is not integer");
			}
			struct expty hi = transExp(venv, tenv, a->u.forr.hi);
			if(hi.ty->kind != Ty_int) {
				EM_error(a->u.forr.hi->pos, "for exp's range type is not integer");
			}
			struct expty body = transExp(venv, tenv, a->u.forr.body);
			if(body.ty->kind != Ty_void) {
				EM_error(a->u.forr.body->pos, "for body must produce no value");
			}
            S_endScope(venv);
			return expTy(NULL, Ty_Void());
		}
		case A_breakExp: {
			return expTy(NULL, Ty_Void());
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
				if(init.ty->kind != actual_ty(arrayType->u.array)->kind) {
					EM_error(a->u.array.init->pos, "array init type don't match");
				}
				return expTy(NULL, Ty_Array(actual_ty(arrayType->u.array)));
			}
			else {
				EM_error(a->pos, "undefined aray type");
				return expTy(NULL, Ty_Void());
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
				S_enter(tenv, tylist->head->name, Ty_Name(tylist->head->name, NULL));
			}
			for(A_nametyList tylist = d->u.type; tylist; tylist = tylist->tail) {
				Ty_ty ty = transTy(tenv, tylist->head->ty);
				if(ty == NULL) {
					EM_error(tylist->head->ty->pos, "undefined type name");
				}
				else {
					Ty_ty t = S_look(tenv, tylist->head->name);
					t->u.name.ty = ty;
				}
			}
			for(A_nametyList tylist = d->u.type; tylist; tylist = tylist->tail) {
				Ty_ty t = S_look(tenv, tylist->head->name);
				if(t->kind == Ty_name) {
					Ty_ty check = t->u.name.ty;
					while(check->kind == Ty_name && check->u.name.sym != t->u.name.sym) {
						check = check->u.name.ty;
					}
					if(check->kind == Ty_name) {
						EM_error(d->pos, "illegal type cycle");
					}
				}
			}
			break;
		}
		case A_functionDec: {
			for(A_fundecList funList = d->u.function; funList; 
			funList = funList->tail) {
				A_fundec f = funList->head;
				Ty_ty resultTy = NULL;
				if(f->result == NULL) {
					resultTy = Ty_Void();
				}
				else {
					resultTy = S_look(tenv, f->result);
					if(resultTy == NULL) {
						EM_error(f->pos, "undefined result type");
					}
				}
				Ty_tyList formalTys = NULL;
				if(f->params == NULL) {
					formalTys = Ty_TyList(NULL, NULL);
				}
				else {
					formalTys = maketylist(tenv, f->params);
				}
				S_enter(venv, f->name, E_FunEntry(formalTys, resultTy));
			}
			for(A_fundecList funList = d->u.function; funList; 
			funList = funList->tail) {
				A_fundec f = funList->head;
				S_beginScope(venv);
				A_fieldList params = f->params;
				if(params != NULL) {
					Ty_tyList paramTys = maketylist(tenv, f->params);
					for(; params; params = params->tail, paramTys = paramTys->tail) {
						S_enter(venv, params->head->name, E_VarEntry(paramTys->head));
					}
				}
				struct expty body = transExp(venv, tenv, f->body);
				Ty_ty resultTy = NULL;
				if(f->result == NULL) {
					resultTy = Ty_Void();
				}
				if(actual_ty(body.ty)->kind != actual_ty(resultTy)->kind) {
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
			Ty_tyList tys = maketylist(tenv, a->u.record);
			Ty_fieldList fields = NULL;
			A_fieldList afields = a->u.record;
			for(; tys; tys = tys->tail, afields = afields->tail) {
				fields = Ty_FieldList(Ty_Field(afields->head->name, tys->head), fields);
			}
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

Ty_ty actual_ty(Ty_ty ty) {
	while(ty->kind == Ty_name) {
		ty = ty->u.name.ty;
	}
	return ty;
}

void SEM_transProg(A_exp exp)
{
	S_table venv = E_base_venv();
	S_table tenv = E_base_tenv();
	transExp(venv, tenv, exp);
}
