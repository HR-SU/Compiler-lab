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
#include "translate.h"

/*Lab5: Your implementation of lab5.*/
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
bool istypematch(Ty_ty ty1, Ty_ty ty2);

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label)
{
	switch(v->kind) {
		case A_simpleVar: {
			E_enventry var = S_look(venv, v->u.simple);
			if(var && var->kind == E_varEntry) {
				Tr_exp trexp = Tr_simpleVar(var->u.var.access, l);
				return expTy(trexp, actual_ty(var->u.var.ty));
			}
			EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
            return expTy(Tr_void(), Ty_Void());
		}
		case A_fieldVar: {
			struct expty var = transVar(venv, tenv, v->u.field.var, l, label);
			if(var.ty->kind == Ty_record) {
				int num = 0;
				for(Ty_fieldList fields = var.ty->u.record; fields;
				fields = fields->tail) {
					if(v->u.field.sym == fields->head->name) {
						return expTy(Tr_fieldVar(var.exp, num), actual_ty(fields->head->ty));
					}
					num++;
				}
				EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
				return expTy(Tr_void(), Ty_Void());
			}
			else {
				EM_error(v->pos, "not a record type");
				return expTy(Tr_void(), Ty_Void());
			}
		}
		case A_subscriptVar: {
			struct expty var = transVar(venv, tenv, v->u.subscript.var, l, label);
			if(var.ty->kind == Ty_array) {
				struct expty exp = transExp(venv, tenv, v->u.subscript.exp, l, label);
				if(exp.ty->kind != Ty_int) {
					EM_error(v->u.subscript.exp->pos, "integer required");
					return expTy(Tr_void(), Ty_Void());
				}
				return expTy(Tr_subscriptVar(var.exp, exp.exp), actual_ty(var.ty->u.array));
			}
			else {
				EM_error(v->pos, "array type required");
				return expTy(Tr_void(), Ty_Void());
			}
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label)
{
	switch(a->kind) {
		case A_varExp: {
			struct expty var = transVar(venv, tenv, a->u.var, l, label);
			return expTy(var.exp, var.ty);
		}
		case A_nilExp: {
			return expTy(Tr_void(), Ty_Nil());
		}
		case A_intExp: {
			return expTy(Tr_intExp(a->u.intt), Ty_Int());
		}
		case A_stringExp: {
			return expTy(Tr_stringExp(a->u.stringg), Ty_String());
		}
		case A_callExp: {
			E_enventry func = S_look(venv, a->u.call.func);
			if(func && func->kind == E_funEntry) {
				A_expList args = a->u.call.args;
				Tr_expList trlist = NULL;
				for(Ty_tyList formal = func->u.fun.formals; formal; formal = formal->tail) {
					if(formal->head == NULL) break;
					if(args == NULL) {
						EM_error(a->pos, "args number don't match");
						return expTy(Tr_void(), actual_ty(func->u.fun.result));
					}
					A_exp arg = args->head;
					struct expty argty = transExp(venv, tenv, arg, l, label);
					if(!istypematch(argty.ty, formal->head)) {
						EM_error(arg->pos, "para type mismatch");
					}
					trlist = Tr_ExpList(argty.exp, trlist);
					args = args->tail;
				}
                if(args != NULL) {
					EM_error(args->head->pos, "too many params in function %s", S_name(a->u.call.func));
				}
				return expTy(Tr_callExp(l, func->u.fun.level, func->u.fun.label, trlist),
					actual_ty(func->u.fun.result));
			}
			else {
				EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
				return expTy(Tr_void(), Ty_Void());
			}
		}
		case A_opExp: {
			struct expty left = transExp(venv, tenv, a->u.op.left, l, label);
			struct expty right = transExp(venv, tenv, a->u.op.right, l, label);
			switch(a->u.op.oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp: {
					if(left.ty->kind != Ty_int) {
						EM_error(a->u.op.left->pos, "integer required");
						return expTy(Tr_void(), Ty_Int());
					}
					if(right.ty->kind != Ty_int) {
						EM_error(a->u.op.right->pos, "integer required");
						return expTy(Tr_void(), Ty_Int());
					}
					return expTy(Tr_opExp(left.exp, right.exp, a->u.op.oper), Ty_Int());
				}
				case A_ltOp:
				case A_leOp:
				case A_gtOp:
				case A_geOp: {
					if(left.ty->kind != Ty_int && left.ty->kind != Ty_string) {
						EM_error(a->u.op.left->pos, "integer or string required");
						return expTy(Tr_void(), Ty_Int());
					}
					if(right.ty->kind != Ty_int&& right.ty->kind != Ty_string) {
						EM_error(a->u.op.right->pos, "integer or string required");
						return expTy(Tr_void(), Ty_Int());
					}
					if(left.ty->kind != right.ty->kind) {
						EM_error(a->u.op.right->pos, "same type required");
					}
					return expTy(Tr_bicmpExp(left.exp, right.exp, a->u.op.oper), Ty_Int());
				}
				case A_eqOp:
				case A_neqOp: {
					if(left.ty->kind != Ty_int && left.ty->kind != Ty_string &&
					left.ty->kind != Ty_record && left.ty->kind != Ty_array &&
					left.ty->kind != Ty_nil) {
						EM_error(a->u.op.left->pos, "integer or string or record or array required");
						return expTy(Tr_void(), Ty_Int());
					}
					if(right.ty->kind != Ty_int&& right.ty->kind != Ty_string &&
					right.ty->kind != Ty_record && right.ty->kind != Ty_array &&
					right.ty->kind != Ty_nil) {
						EM_error(a->u.op.right->pos, "integer or string or record or array required");
						return expTy(Tr_void(), Ty_Int());
					}
					if(!istypematch(left.ty, right.ty)) {
						EM_error(a->u.op.right->pos, "same type required");
					}
					return expTy(Tr_bicmpExp(left.exp, right.exp, a->u.op.oper), Ty_Int());
				}
				default: {
					EM_error(a->pos, "undefined operator");
					return expTy(Tr_void(), Ty_Int());
				}
			}
		}
		case A_recordExp: {
			Ty_ty recordType = S_look(tenv, a->u.record.typ);
            if(recordType) recordType = actual_ty(recordType);
			if(recordType && recordType->kind == Ty_record) {
				Tr_expList trlist = NULL;
				int size = 0;
				for(Ty_fieldList f = recordType->u.record; f; f = f->tail) {
					bool found = FALSE;
					for(A_efieldList fields = a->u.record.fields; fields;
					fields = fields->tail) {
						if(fields->head->name == f->head->name) {
							found = TRUE;
							struct expty exp = transExp(venv, tenv, fields->head->exp, l, label);
							trlist = Tr_ExpList(exp.exp, trlist);
							if(!istypematch(exp.ty, f->head->ty)) {
								EM_error(fields->head->exp->pos, "field type don't match");
							}
                            break;
						}
						if(fields->tail == NULL && !found) {
							EM_error(fields->head->exp->pos, "undefined field name");
						}
					}
					size++;
				}
				return expTy(Tr_recordExp(trlist, size), recordType);
			}
			else {
				EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
				return expTy(Tr_void(), Ty_Nil());
			}
		}
		case A_seqExp: {
			Tr_exp trexp = NULL;
			for(A_expList expList = a->u.seq; expList; expList = expList->tail) {
				if(expList == NULL) return expTy(Tr_void(), Ty_Void());
				struct expty exp = transExp(venv, tenv, expList->head, l, label);
				trexp = Tr_seqExp(trexp, exp.exp);
				if(expList->tail == NULL) {
					return expTy(trexp, exp.ty);
				}
			}
			return expTy(Tr_void(), Ty_Void());
		}
		case A_assignExp: {
			struct expty var = transVar(venv, tenv, a->u.assign.var, l, label);
			if(a->u.assign.var->kind == A_simpleVar) {
				E_enventry v = S_look(venv, a->u.assign.var->u.simple);
				if(v->readonly) {
					EM_error(a->u.assign.var->pos, "loop variable can't be assigned");
					return expTy(Tr_void(), Ty_Void());
				}
			}
			struct expty exp = transExp(venv, tenv, a->u.assign.exp, l, label);
			if(!istypematch(var.ty, exp.ty)) {
				EM_error(a->pos, "unmatched assign exp");
			}
			return expTy(Tr_assignExp(var.exp, exp.exp), Ty_Void());
		}
		case A_ifExp: {
			struct expty test = transExp(venv, tenv, a->u.iff.test, l, label);
			if(test.ty->kind != Ty_int) {
				EM_error(a->u.iff.test->pos, "integer required");
			}
			struct expty then = transExp(venv, tenv, a->u.iff.then, l, label);
			struct expty elsee;
			if(a->u.iff.elsee == NULL) elsee = expTy(NULL, Ty_Void());
			else elsee = transExp(venv, tenv, a->u.iff.elsee, l, label);
			if(elsee.ty->kind == Ty_void && then.ty->kind != Ty_void) {
				EM_error(a->u.iff.then->pos, "if-then exp's body must produce no value");
				return expTy(Tr_void(), Ty_Void());
			}
			if(!istypematch(then.ty, elsee.ty)) {
				EM_error(a->u.iff.elsee->pos, "then exp and else exp type mismatch");
			}
			return expTy(Tr_ifExp(test.exp, then.exp, elsee.exp), actual_ty(then.ty));
		}
		case A_whileExp: {
			Temp_label done = Temp_newlabel();
			struct expty test = transExp(venv, tenv, a->u.whilee.test, l, label);
			if(test.ty->kind != Ty_int) {
				EM_error(a->u.whilee.test->pos, "integer required");
			}
			struct expty body = transExp(venv, tenv, a->u.whilee.body, l, done);
			if(body.ty->kind != Ty_void) {
				EM_error(a->u.whilee.body->pos, "while body must produce no value");
			}
			return expTy(Tr_whileExp(test.exp, body.exp, done), Ty_Void());
		}
		case A_forExp: {
			Temp_label done = Temp_newlabel();
            S_beginScope(venv);
            S_enter(venv, a->u.forr.var, E_ROVarEntry(Tr_allocLocal(l, TRUE), Ty_Int()));
			struct expty lo = transExp(venv, tenv, a->u.forr.lo, l, label);
			if(lo.ty->kind != Ty_int) {
				EM_error(a->u.forr.lo->pos, "for exp's range type is not integer");
			}
			struct expty hi = transExp(venv, tenv, a->u.forr.hi, l, label);
			if(hi.ty->kind != Ty_int) {
				EM_error(a->u.forr.hi->pos, "for exp's range type is not integer");
			}
			struct expty body = transExp(venv, tenv, a->u.forr.body, l, done);
			if(body.ty->kind != Ty_void) {
				EM_error(a->u.forr.body->pos, "for body must produce no value");
			}
            S_endScope(venv);
			return expTy(Tr_forExp(lo.exp, hi.exp, body.exp, done), Ty_Void());
		}
		case A_breakExp: {
			return expTy(Tr_breakExp(label), Ty_Void());
		}
		case A_letExp: {
			struct expty exp;
			A_decList dec;
			Tr_exp trdec;
			Tr_expList trlist = NULL;
			S_beginScope(venv);
			S_beginScope(tenv);
			for(dec = a->u.let.decs; dec; dec = dec->tail) {
				trdec = transDec(venv, tenv, dec->head, l, label);
				trlist = Tr_ExpList(trdec, trlist);
			}
			exp = transExp(venv, tenv, a->u.let.body, l, label);
			S_endScope(tenv);
			S_endScope(venv);
			return expTy(Tr_letExp(trlist, exp.exp), exp.ty);
		}
		case A_arrayExp: {
			Ty_ty arrayType = actual_ty(S_look(tenv, a->u.array.typ));
			if(arrayType && arrayType->kind == Ty_array) {
				struct expty size = transExp(venv, tenv, a->u.array.size, l, label);
				if(size.ty->kind != Ty_int) {
					EM_error(a->u.array.size->pos, "integer required");
				}
				struct expty init = transExp(venv, tenv, a->u.array.init, l, label);
				if(!istypematch(init.ty, arrayType->u.array)) {
					EM_error(a->u.array.init->pos, "type mismatch");
				}
				return expTy(Tr_arrayExp(size.exp, init.exp), arrayType);
			}
			else {
				EM_error(a->pos, "undefined aray type");
				return expTy(Tr_void(), Ty_Void());
			}
		}
	}
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label label)
{
	switch(d->kind) {
		case A_varDec: {
			Tr_access access;
			struct expty init = transExp(venv, tenv, d->u.var.init, l, label);
			if(d->u.var.typ != NULL) {
				Ty_ty ty = S_look(tenv, d->u.var.typ);
				if(ty->kind == Ty_nil) {
					EM_error(d->pos, "cannot declare a var with type nil");
				}
				if(!istypematch(init.ty, ty)) {
					EM_error(d->pos, "type mismatch");
				}
				access = Tr_allocLocal(l, TRUE);
				S_enter(venv, d->u.var.var, E_VarEntry(access, ty));
			}
			else {
				if(init.ty->kind == Ty_nil) {
					EM_error(d->pos, "init should not be nil without type specified");
				}
				access = Tr_allocLocal(l, TRUE);
				S_enter(venv, d->u.var.var, E_VarEntry(access, init.ty));
			}
			return Tr_varDec(access, init.exp);
		}
		case A_typeDec: {
			for(A_nametyList tylist = d->u.type; tylist; tylist = tylist->tail) {
				for(A_nametyList check = tylist->tail; check; check = check->tail) {
					if(strcmp(S_name(check->head->name), S_name(tylist->head->name)) == 0) {
						EM_error(tylist->head->ty->pos, "two types have the same name");
						return Tr_void();
					}
				}
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
					Ty_ty check = t->u.name.ty, tmp;
					while(check->kind == Ty_name && check->u.name.sym != t->u.name.sym) {
						check = check->u.name.ty;
					}
					if(check->kind == Ty_name) {
						EM_error(d->pos, "illegal type cycle");
						return Tr_void();
					}
				}
			}
			return Tr_void();
		}
		case A_functionDec: {
			for(A_fundecList funList = d->u.function; funList; 
			funList = funList->tail) {
				for(A_fundecList check = funList->tail; check; check = check->tail) {
					if(strcmp(S_name(check->head->name), S_name(funList->head->name)) == 0) {
						EM_error(funList->head->pos, "two functions have the same name");
						return Tr_void();
					}
				}
				A_fundec f = funList->head;
				Ty_ty resultTy = NULL;
				if(f->result == NULL) {
					resultTy = Ty_Void();
				}
				else {
					resultTy = actual_ty(S_look(tenv, f->result));
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
				Temp_label funLabel = Temp_newlabel();
				Tr_level newLevel = Tr_newLevel(l, funLabel, NULL);
				S_enter(venv, f->name, E_FunEntry(newLevel, funLabel, formalTys, resultTy));
			}
			for(A_fundecList funList = d->u.function; funList; 
			funList = funList->tail) {
				A_fundec f = funList->head;
				S_beginScope(venv);
				A_fieldList params = f->params;
				if(params != NULL) {
					Ty_tyList paramTys = maketylist(tenv, f->params);
					for(; params; params = params->tail, paramTys = paramTys->tail) {
						S_enter(venv, params->head->name, E_VarEntry(Tr_allocLocal(l, TRUE), paramTys->head));
					}
				}
				E_enventry funEntry = S_look(venv, f->name);
				struct expty body = transExp(venv, tenv, f->body, funEntry->u.fun.level, label);
				if(f->result == NULL && body.ty->kind != Ty_void) {
					EM_error(f->body->pos, "procedure returns value");
				}
				else if(f->result){
					Ty_ty resultTy = actual_ty(S_look(tenv, f->result));
					if(!istypematch(body.ty, resultTy)) {
						EM_error(f->body->pos, "return type don't match");
					}
				}
				S_endScope(venv);
				Tr_procEntryExit(body.exp, funEntry->u.fun.level);
			}
			return Tr_void();
		}
		default: {
			EM_error(d->pos, "error");
		}
	}
	return Tr_void();
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
	if(flist == NULL) {
		return NULL;
	}
    Ty_tyList tlist = maketylist(tenv, flist->tail);
	Ty_ty ty = S_look(tenv, flist->head->typ);
	if(ty == NULL) {
		EM_error(flist->head->pos, "undefined type %s", S_name(flist->head->typ));
		ty = Ty_Void();
	}
	return Ty_TyList(ty, tlist);
}

Ty_ty actual_ty(Ty_ty ty) {
	if(ty == NULL) return Ty_Void();
	while(ty->kind == Ty_name) {
		ty = ty->u.name.ty;
	}
	return ty;
}

bool istypematch(Ty_ty _ty1, Ty_ty _ty2) {
	Ty_ty ty1 = actual_ty(_ty1);
	Ty_ty ty2 = actual_ty(_ty2);
	if(ty1->kind == ty2->kind) {
		if(ty1->kind == Ty_record) return ty1 == ty2;
		if(ty1->kind == Ty_array) return ty1 == ty2;
		return TRUE;
	}
	else if(ty1->kind == Ty_nil && ty2->kind == Ty_record) return TRUE;
	else if(ty1->kind == Ty_record && ty2->kind == Ty_nil) return TRUE;
	return FALSE;
}

F_fragList SEM_transProg(A_exp exp){

	//TODO LAB5: do not forget to add the main frame
	S_table tenv = E_base_tenv(), venv = E_base_venv();
	struct expty main = transExp(venv, tenv, exp, Tr_outermost(), Temp_newlabel());
	Tr_procEntryExit(main.exp, Tr_outermost());
	return Tr_getResult();
}

