#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.

static F_fragList fragList = NULL;

struct Tr_access_ {
	Tr_level level;
	F_access access;
};


// struct Tr_accessList_ {
// 	Tr_access head;
// 	Tr_accessList tail;	
// };

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};

typedef struct patchList_ *patchList;
struct patchList_ 
{
	Temp_label *head; 
	patchList tail;
};

struct Cx 
{
	patchList trues; 
	patchList falses; 
	T_stm stm;
};

struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx; } u;
};

struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
	Tr_expList expList = checked_malloc(sizeof(*expList));
	expList->head = head;
	expList->tail = tail;
	return expList;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
	Tr_accessList accessList = checked_malloc(sizeof(*accessList));
	accessList->head = head;
	accessList->tail = tail;
	return accessList;
}

static patchList PatchList(Temp_label *head, patchList tail)
{
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}

void doPatch(patchList tList, Temp_label label)
{
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}

static T_exp unEx(Tr_exp e) {
	switch(e->kind) {
		case Tr_ex:
			return e->u.ex;
		case Tr_cx: {
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel();
			Temp_label f = Temp_newlabel();
			doPatch(e->u.cx.trues, t);
			doPatch(e->u.cx.falses, f);
			return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
				T_Eseq(e->u.cx.stm,
				T_Eseq(T_Label(f),
				T_Eseq(T_Move(T_Temp(r), T_Const(0)),
				T_Eseq(T_Label(t), T_Temp(r))))));
		}
		case Tr_nx:
			return T_Eseq(e->u.nx, T_Const(0));
		default: assert(0);
	}
}

static T_stm unNx(Tr_exp e) {
	switch(e->kind) {
		case Tr_nx:
			return e->u.nx;
		case Tr_ex:
			return T_Exp(e->u.ex);
		case Tr_cx:
			return T_Exp(unEx(e));
		default: assert(0);
	}
}

static struct Cx unCx(Tr_exp e) {
	switch(e->kind) {
		case Tr_cx:
			return e->u.cx;
		case Tr_ex: {
			T_stm cjump = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
			patchList trues = PatchList(&cjump->u.CJUMP.true, NULL);
			patchList falses = PatchList(&cjump->u.CJUMP.false, NULL);
			struct Cx cx;
			cx.stm = cjump;
			cx.trues = trues;
			cx.falses = falses;
			return cx;
		}
		default: assert(0);
	}
}

static struct Tr_level_ outermostLevel = {NULL, NULL};

Tr_level Tr_outermost(void) {
	if(outermostLevel.frame == NULL) {
		Temp_label name = Temp_namedlabel("main");
		F_frame frame = F_newFrame(name, NULL);
		outermostLevel.frame = frame;
	}
	return &outermostLevel;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
	U_boolList newFormals = U_BoolList(TRUE, formals);
	F_frame frame = F_newFrame(name, newFormals);
	Tr_level level = checked_malloc(sizeof(*level));
	level->frame = frame;
	level->parent = parent;
	return level;
}

Tr_accessList Tr_makeAccessList(F_accessList flist, Tr_level level) {
	if(flist == NULL) return NULL;
	Tr_accessList tail = Tr_makeAccessList(flist->tail, level);
	Tr_access access = checked_malloc(sizeof(*access));
	access->level = level;
	access->access = flist->head;
	return Tr_AccessList(access, tail);
}

Tr_accessList Tr_formals(Tr_level level) {
	F_accessList flist = F_formals(level->frame);
	Tr_accessList accessList = Tr_makeAccessList(flist->tail, level);
	return accessList;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
	Tr_access access = checked_malloc(sizeof(*access));
	access->access = F_allocLocal(level->frame, escape);
	access->level = level;
	return access;
}

Tr_exp Tr_void() {
	Tr_exp exp = checked_malloc(sizeof(*exp));
	exp->kind = Tr_ex;
	exp->u.ex = T_Const(0);
	return exp;
}
Tr_exp Tr_simpleVar(Tr_access access, Tr_level level) {
	Tr_exp exp = checked_malloc(sizeof(*exp));
	exp->kind = Tr_ex;
	T_exp texp;
	if(access->level != level) {
		texp = T_Mem(T_Binop(T_plus, T_Const(16), T_Temp(F_FP())));
		level = level->parent;
		while(access->level != level) {
			texp = T_Mem(T_Binop(T_plus, T_Const(16), texp));
			level = level->parent;
		}
	}
	else {
		texp = T_Temp(F_FP());
	}
	exp->u.ex = F_Exp(access->access, texp);
	return exp;
}

Tr_exp Tr_fieldVar(Tr_exp var, int num) {
	T_exp exp = T_Mem(T_Binop(T_plus, unEx(var), T_Const(num*8)));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_subscriptVar(Tr_exp var, Tr_exp offset) {
	T_exp exp = T_Mem(T_Binop(T_plus, unEx(var), T_Binop(T_mul, unEx(offset), T_Const(8))));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_intExp(int val) {
	T_exp exp = T_Const(val);
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_stringExp(string str) {
	Temp_label label = Temp_newlabel();
	T_exp exp = T_Name(label);
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	F_frag frag = F_StringFrag(label, str);
	fragList = F_FragList(frag, fragList);
	return ret;
}

T_expList makeArgList(Tr_expList args) {
	T_expList t = NULL;
	for(; args; args = args->tail) {
		t = T_ExpList(unEx(args->head), t);
	}
	return t;
}

Tr_exp Tr_callExp(Tr_level crtLevel, Tr_level level, Temp_label label, Tr_expList args) {
	T_exp sl = T_Temp(F_FP());
	while(level->parent != crtLevel) {
		sl = T_Mem(T_Binop(T_plus, sl, T_Const(16)));
		crtLevel = crtLevel->parent;
	}
	T_exp exp = T_Call(T_Name(label), T_ExpList(sl, makeArgList(args)));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_opExp(Tr_exp left, Tr_exp right, A_oper op) {
	T_binOp binop;
	switch(op) {
		case A_plusOp: binop = T_plus; break;
		case A_minusOp: binop = T_minus; break;
		case A_timesOp: binop = T_mul; break;
		case A_divideOp: binop = T_div; break;
	}
	T_exp exp = T_Binop(binop, unEx(left), unEx(right));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_bicmpExp(Tr_exp left, Tr_exp right, A_oper op) {
	T_relOp relop;
	switch(op) {
		case A_ltOp: relop = T_lt; break;
		case A_leOp: relop = T_le; break;
		case A_gtOp: relop = T_gt; break;
		case A_geOp: relop = T_ge; break;
		case A_eqOp: relop = T_eq; break;
		case A_neqOp: relop = T_ne; break;
	}
	T_stm cmp = T_Cjump(relop, unEx(left), unEx(right), NULL, NULL);
	patchList trues = PatchList(&cmp->u.CJUMP.true, NULL);
	patchList falses = PatchList(&cmp->u.CJUMP.false, NULL);
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_cx;
	ret->u.cx.stm = cmp;
	ret->u.cx.trues = trues;
	ret->u.cx.falses =falses;
	return ret;
}

T_stm makeRecordTree(Tr_expList expList, Temp_temp tmp, int offset) {
	T_stm stm = T_Move(T_Mem(T_Binop(T_plus, T_Temp(tmp), T_Const(offset))), 
		unEx(expList->head));
	if(expList->tail == NULL) {
		return stm;
	}
	else {
		T_stm next = makeRecordTree(expList->tail, tmp, offset-8);
		return T_Seq(stm, next);
	}
}

Tr_exp Tr_recordExp(Tr_expList expList, int size) {
	Temp_temp tmp = Temp_newtemp();
	T_stm next = makeRecordTree(expList, tmp, (size-1)*8);
	T_stm alloc = T_Move(T_Temp(tmp),
		T_Call(T_Name(Temp_namedlabel("malloc")), T_ExpList(T_Const(size*8), NULL)));
	T_exp exp = T_Eseq(T_Seq(alloc, next), T_Temp(tmp));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_seqExp(Tr_exp first, Tr_exp second) {
	T_exp exp;
	if(first == NULL) exp = unEx(second);
	else exp = T_Eseq(T_Exp(unEx(first)), unEx(second));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp val) {
	T_stm stm = T_Move(unEx(var), unEx(val));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_nx;
	ret->u.nx = stm;
	return ret;
}

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee) {
	Temp_label t = Temp_newlabel(), f = Temp_newlabel();
	struct Cx c = unCx(test);
	doPatch(c.trues, t);
	doPatch(c.falses, f);
	Tr_exp ret = checked_malloc(sizeof(*ret));
	if(elsee == NULL) {
		T_stm stm = T_Seq(c.stm,
			T_Seq(T_Label(t),
			T_Seq(unNx(then), T_Label(f))));
		ret->kind = Tr_nx;
		ret->u.nx = stm;
		return ret;
	}
	else {
		Temp_temp val = Temp_newtemp();
		Temp_label result = Temp_newlabel();
		T_exp texp = unEx(then), fexp = unEx(elsee);
		T_exp exp = T_Eseq(c.stm,
			T_Eseq(T_Label(t),
			T_Eseq(T_Move(T_Temp(val), texp),
			T_Eseq(T_Jump(T_Name(result), Temp_LabelList(result, NULL)),
			T_Eseq(T_Label(f),
			T_Eseq(T_Move(T_Temp(val), fexp),
			T_Eseq(T_Label(result),T_Temp(val))))))));
		ret->kind = Tr_ex;
		ret->u.ex = exp;
	}
}

Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label done) {
	Temp_label t = Temp_newlabel(), b = Temp_newlabel();
	T_stm cjump = T_Cjump(T_eq, T_Const(0), unEx(test), done, b);
	T_stm jump = T_Jump(T_Name(t), Temp_LabelList(t, NULL));
	T_stm stm = T_Seq(T_Label(t),
		T_Seq(cjump,
		T_Seq(T_Label(b),
		T_Seq(unNx(body),
		T_Seq(jump, T_Label(done))))));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_nx;
	ret->u.nx = stm;
	return ret;
}

Tr_exp Tr_breakExp(Temp_label done) {
	T_stm stm = T_Jump(T_Name(done), Temp_LabelList(done, NULL));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_nx;
	ret->u.nx = stm;
	return ret;
}

Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done) {
	Temp_temp i = Temp_newtemp();
	Temp_temp limit = Temp_newtemp();
	T_stm init = T_Seq(T_Move(T_Temp(i), unEx(lo)), T_Move(T_Temp(limit), unEx(hi)));
	Temp_label test = Temp_newlabel(), b = Temp_newlabel();
	T_stm cjump = T_Cjump(T_gt, T_Temp(i), T_Temp(limit), done, b);
	T_stm jump = T_Jump(T_Name(test), Temp_LabelList(test, NULL));
	T_stm realBody = T_Seq(unNx(body), T_Exp(T_Binop(T_plus, T_Temp(i), T_Const(1))));
	T_stm stm = T_Seq(init,
		T_Seq(T_Label(test),
		T_Seq(cjump,
		T_Seq(T_Label(b),
		T_Seq(realBody,
		T_Seq(jump, T_Label(done)))))));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_nx;
	ret->u.nx = stm;
	return ret;
}

Tr_exp Tr_letExp(Tr_expList decs, Tr_exp body) {
	T_stm decStm = T_Exp(T_Const(0));
	for(; decs; decs = decs->tail) {
		decStm = T_Seq(unNx(decs->head), decStm);
	}
	T_exp exp = T_Eseq(decStm, unEx(body));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) {
	Temp_temp tmp = Temp_newtemp();
	T_exp exp = T_Eseq(T_Move(T_Temp(tmp),
		T_Call(T_Name(Temp_namedlabel("initArray")), 
			T_ExpList(unEx(init), 
			T_ExpList(unEx(size), NULL)))), T_Temp(tmp));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_ex;
	ret->u.ex = exp;
	return ret;
}

Tr_exp Tr_varDec(Tr_access access, Tr_exp init) {
	T_exp var = F_Exp(access->access, T_Temp(F_FP()));
	T_stm stm = T_Move(var, unEx(init));
	Tr_exp ret = checked_malloc(sizeof(*ret));
	ret->kind = Tr_nx;
	ret->u.nx = stm;
	return ret;
}

void Tr_procEntryExit(Tr_exp body, Tr_level level) {
	T_stm newBody = T_Move(T_Temp(F_RV()), unEx(body));
	T_stm stm1 = F_procEntryExit1(level->frame, newBody);
	F_frag frag = F_ProcFrag(stm1, level->frame);
	fragList = F_FragList(frag, fragList);
}

F_fragList Tr_getResult(void) {
	return fragList;
}
