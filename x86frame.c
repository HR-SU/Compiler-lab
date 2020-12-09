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
};

static Temp_temp fp = NULL;
static Temp_temp rv = NULL;
static Temp_temp di = NULL;
static Temp_temp si = NULL;
static Temp_temp dx = NULL;
static Temp_temp cx = NULL;
static Temp_temp r8 = NULL;
static Temp_temp r9 = NULL;
static Temp_temp r10 = NULL;
static Temp_temp r11 = NULL;

Temp_temp F_FP(void) {
	if(fp == NULL) fp = Temp_newtemp();
	return fp;
}

Temp_temp F_RV(void) {
	if(rv == NULL) rv = Temp_newtemp();
	return rv;
}

Temp_temp F_DX(void) {
	if(dx == NULL) dx = Temp_newtemp();
	return dx;
}

Temp_temp F_ARG(int i) {
	switch(i) {
		case 0: {
			if(di == NULL) di = Temp_newtemp();
			return di;
		}
		case 1: {
			if(si == NULL) di = Temp_newtemp();
			return si;
		}
		case 2: {
			if(dx == NULL) dx = Temp_newtemp();
			return dx;
		}
		case 3: {
			if(cx == NULL) cx = Temp_newtemp();
			return cx;
		}
		case 4: {
			if(r8 == NULL) r8 = Temp_newtemp();
			return r8;
		}
		case 5: {
			if(r9 == NULL) r9 = Temp_newtemp();
			return r9;
		}
		default: {
			return Temp_newtemp();
		}
	}
}

Temp_tempList F_calldefs() {
	if(r10 == NULL) r10 = Temp_newtemp();
	if(r11 == NULL) r11 = Temp_newtemp();
	return Temp_TempList(F_RV(), Temp_TempList(r10, Temp_TempList(r11, NULL)));
}

F_accessList makeAccessList(U_boolList boolList, int offset) {
	if(boolList == NULL) return NULL;
	F_accessList accesslist;
	F_access access = checked_malloc(sizeof(*access));
	if(boolList->head) {
		accesslist = makeAccessList(boolList->tail, offset+8);
		access->kind = inFrame;
		access->u.offset = offset;
	}
	else {
		accesslist = makeAccessList(boolList->tail, offset);
		access->kind = inReg;
		access->u.reg = Temp_newtemp();
	}
	return F_AccessList(access, accesslist);
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame f = checked_malloc(sizeof(*f));
	f->name = name;
	F_accessList formalAccessList = makeAccessList(formals, 16);
	f->formals = formalAccessList;
	f->size = 0;
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
		access->u.offset = -f->size;
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

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
	return stm;
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

