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
};

static Temp_temp fp = NULL;
static Temp_temp rv = NULL;

Temp_temp F_FP(void) {
	if(fp == NULL) fp = Temp_newtemp();
	return fp;
}

Temp_temp F_RV(void) {
	if(rv == NULL) rv = Temp_newtemp();
	return rv;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame f = checked_malloc(sizeof(*f));
	f->name = name;
	F_accessList formalAccessList = makeAccessList(formals);
	f->formals = formalAccessList;
	return f;
}

F_accessList makeAccessList(U_boolList boolList) {
	if(boolList = NULL) return NULL;
	F_accessList accesslist = makeAccessList(boolList->tail);
	F_access access = checked_malloc(sizeof(*access));
	if(boolList->head) access->kind = inFrame;
	else access->kind = inReg;
	return F_AccessList(access, accesslist);
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
		access->u.offset = 0;
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
	if(access->kind = inFrame) {
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

