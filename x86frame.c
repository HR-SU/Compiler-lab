#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

struct F_frame_ {
	Temp_label name;
	F_accessList formals;
	F_accessList locals;
	int size;
	T_stm init;
};

static Temp_temp rv = NULL;
static Temp_temp bx = NULL;
static Temp_temp cx = NULL;
static Temp_temp dx = NULL;
static Temp_temp si = NULL;
static Temp_temp di = NULL;
static Temp_temp fp = NULL;
static Temp_temp sp = NULL;
static Temp_temp r8 = NULL;
static Temp_temp r9 = NULL;
static Temp_temp r10 = NULL;
static Temp_temp r11 = NULL;
static Temp_temp r12 = NULL;
static Temp_temp r13 = NULL;
static Temp_temp r14 = NULL;
static Temp_temp r15 = NULL;

static Temp_tempList callerSaved = NULL;
static Temp_tempList calleeSaved = NULL;
static Temp_tempList registers = NULL;
static Temp_tempList returnSink = NULL;

void F_init() {
	if(rv == NULL) rv = Temp_newtemp();
	if(bx == NULL) bx = Temp_newtemp();
	if(cx == NULL) cx = Temp_newtemp();
	if(dx == NULL) dx = Temp_newtemp();
	if(si == NULL) si = Temp_newtemp();
	if(di == NULL) di = Temp_newtemp();
	if(fp == NULL) fp = Temp_newtemp();
	if(sp == NULL) sp = Temp_newtemp();
	if(r8 == NULL) r8 = Temp_newtemp();
	if(r9 == NULL) r9 = Temp_newtemp();
	if(r10 == NULL) r10 = Temp_newtemp();
	if(r11 == NULL) r11 = Temp_newtemp();
	if(r12 == NULL) r12 = Temp_newtemp();
	if(r13 == NULL) r13 = Temp_newtemp();
	if(r14 == NULL) r14 = Temp_newtemp();
	if(r15 == NULL) r15 = Temp_newtemp();
	if(callerSaved == NULL) {
		Temp_tempList result = Temp_TempList(r11, NULL);
		result = Temp_TempList(r10, result);
		result = Temp_TempList(r9, result);
		result = Temp_TempList(r8, result);
		result = Temp_TempList(di, result);
		result = Temp_TempList(si, result);
		result = Temp_TempList(dx, result);
		result = Temp_TempList(cx, result);
		result = Temp_TempList(rv, result);
		callerSaved = result;
	}
	if(calleeSaved == NULL) {
		Temp_tempList result = Temp_TempList(r15, NULL);
		result = Temp_TempList(r14, result);
		result = Temp_TempList(r13, result);
		result = Temp_TempList(r12, result);
		result = Temp_TempList(fp, result);
		result = Temp_TempList(bx, result);
		calleeSaved = result;
	}
	if(registers == NULL) {
		Temp_tempList result = Temp_TempList(r15, NULL);
		result = Temp_TempList(r14, result);
		result = Temp_TempList(r13, result);
		result = Temp_TempList(r12, result);
		result = Temp_TempList(r11, result);
		result = Temp_TempList(r10, result);
		result = Temp_TempList(r9, result);
		result = Temp_TempList(r8, result);
		result = Temp_TempList(sp, result);
		result = Temp_TempList(fp, result);
		result = Temp_TempList(bx, result);
		result = Temp_TempList(di, result);
		result = Temp_TempList(si, result);
		result = Temp_TempList(dx, result);
		result = Temp_TempList(cx, result);
		result = Temp_TempList(rv, result);
		registers = result;
	}
	if(returnSink == NULL) {
		returnSink = Temp_TempList(rv, Temp_TempList(sp, calleeSaved));
	}
}

Temp_temp F_FP(void) {
	return fp;
}

Temp_temp F_RV(void) {
	return rv;
}

Temp_temp F_DX(void) {
	return dx;
}

Temp_temp F_SP(void) {
	return sp;
}

Temp_temp F_ARG(int i) {
	switch(i) {
		case 0: return di;
		case 1: return si;
		case 2: return dx;
		case 3: return cx;
		case 4: return r8;
		case 5: return r9;
		default: return Temp_newtemp();
	}
}

Temp_tempList F_calldefs() {
	return callerSaved;
}

Temp_tempList F_registers() {
	return registers;
}

static Temp_map tempMap = NULL;

Temp_map F_TempMap() {
	if(tempMap == NULL) {
		Temp_map map = Temp_empty();
		Temp_enter(map, rv, "%rax");
		Temp_enter(map, bx, "%rbx");
		Temp_enter(map, cx, "%rcx");
		Temp_enter(map, dx, "%rdx");
		Temp_enter(map, si, "%rsi");
		Temp_enter(map, di, "%rdi");
		Temp_enter(map, fp, "%rbp");
		Temp_enter(map, sp, "%rsp");
		Temp_enter(map, r8, "%r8");
		Temp_enter(map, r9, "%r9");
		Temp_enter(map, r10, "%r10");
		Temp_enter(map, r11, "%r11");
		Temp_enter(map, r12, "%r12");
		Temp_enter(map, r13, "%r13");
		Temp_enter(map, r14, "%r14");
		Temp_enter(map, r15, "%r15");
		tempMap = map;
	}
	return tempMap;
}

F_accessList makeAccessList(F_frame f, U_boolList boolList, int num) {
	if(boolList == NULL) return NULL;
	F_accessList accesslist;
	F_access access = checked_malloc(sizeof(*access));
	if(num < 6) {
		if(boolList->head) {
			access->kind = inFrame;
			access->u.offset = -(f->size + 8);
			f->size = f->size + 8;
			accesslist = makeAccessList(f, boolList->tail, num+1);
		}
		else {
			access->kind = inReg;
			access->u.reg = Temp_newtemp();
			accesslist = makeAccessList(f, boolList->tail, num+1);
		}
	}
	else {
		access->kind = inFrame;
		access->u.offset = (num-6)*8+16;
		accesslist = makeAccessList(f, boolList->tail, num+1);
	}
	return F_AccessList(access, accesslist);
}

T_stm initFormals(F_accessList fl, int num) {
	if(fl == NULL) return T_Exp(T_Const(0));
	T_stm stm;
	if(num < 6) {
		if(fl->head->kind == inFrame) {
			stm = T_Move(T_Mem(T_Binop(T_plus,
				T_Const(fl->head->u.offset), T_Temp(F_FP()))), T_Temp(F_ARG(num)));
		}
		else {
			stm = T_Move(T_Temp(fl->head->u.reg), T_Temp(F_ARG(num)));
		}
	}
	if(fl->tail == NULL) return stm;
	else return T_Seq(stm, initFormals(fl->tail, num+1));
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame f = checked_malloc(sizeof(*f));
	f->name = name;
	f->size = 0;
	F_accessList formalAccessList = makeAccessList(f, formals, 0);
	f->formals = formalAccessList;
	f->init = initFormals(formalAccessList, 0);
	return f;
}

Temp_label F_name(F_frame f) {
	return f->name;
}

F_accessList F_formals(F_frame f) {
	return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
	F_access access = checked_malloc(sizeof(*access));
	if(escape) {
		access->kind = inFrame;
		access->u.offset = -(f->size + 8);
		f->size = f->size + 8;
	}
	else {
		access->kind = inReg;
		access->u.reg = Temp_newtemp();
	}
	f->locals = F_AccessList(access, f->locals);
	return access;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
	F_accessList accesslist = checked_malloc(sizeof(*accesslist));
	accesslist->head = head;
	accesslist->tail = tail;
	return accesslist;
}

T_exp F_Exp(F_access access, T_exp framePtr) {
	if(access->kind == inFrame) {
		return T_Mem(T_Binop(T_plus, T_Const(access->u.offset), framePtr));
	}
	else {
		return T_Temp(access->u.reg);
	}
}

T_exp F_externalCall(Temp_label name, T_expList args) {
	return T_Call(T_Name(name), args);
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
	T_stm entry = T_Exp(T_Const(0)), exit = T_Exp(T_Const(0));
	for(Temp_tempList tl = calleeSaved; tl; tl = tl->tail) {
		if(tl->head == fp) continue;
		Temp_temp tmp = Temp_newtemp();
		entry = T_Seq(entry, T_Move(T_Temp(tmp), T_Temp(tl->head)));
		exit = T_Seq(T_Move(T_Temp(tl->head), T_Temp(tmp)), exit);
	}
	return T_Seq(frame->init,
		T_Seq(entry, T_Seq(stm, exit)));
}

AS_instrList F_procEntryExit2(AS_instrList body) {
	return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body) {
	char entry[80];
	sprintf(entry, "push %%rbp\nmovq %%rsp, %%rbp\nsubq $%d, %%rsp\n", frame->size);
	char exit[80];
	sprintf(exit, "addq $%d, %%rsp\npopq %%rbp\nret\n", frame->size);
	return AS_Proc(String(entry), body, String(exit));
}

F_frag F_StringFrag(Temp_label label, string str) {   
	F_frag frag = checked_malloc(sizeof(*frag));
	frag->kind = F_stringFrag;
	frag->u.stringg.label = label;
	frag->u.stringg.str = str;
	return frag;                                    
}                                                     
                                                      
F_frag F_ProcFrag(T_stm body, F_frame frame) {        
	F_frag frag = checked_malloc(sizeof(*frag));
	frag->kind = F_procFrag;
	frag->u.proc.body = body;
	frag->u.proc.frame = frame;
	return frag;                                       
}                                                     
                                                      
F_fragList F_FragList(F_frag head, F_fragList tail) { 
	F_fragList fragList = checked_malloc(sizeof(*fragList));
	fragList->head = head;
	fragList->tail = tail;
	return fragList;                                   
}                                                     

