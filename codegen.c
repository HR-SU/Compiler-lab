#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//Lab 6: put your code here
static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst) {
    if(last != NULL) {
        last = last->tail = AS_InstrList(inst, NULL);
    }
    else last = iList = AS_InstrList(inst, NULL);
}

static Temp_temp munchExp(T_exp e);
static void munchStm(T_stm s);
static Temp_tempList munchArgs(int num, T_expList);

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;
    T_stmList sl;

    for(sl = stmList; sl; sl = sl->tail) munchStm(sl->head);

    list = iList; iList = last = NULL;
    return F_procEntryExit2(list);
}

Temp_temp munchExp(T_exp e) {
    char *s = checked_malloc(40);
    switch(e->kind) {
        case T_MEM: {
            if(e->u.MEM->kind == T_BINOP) {
                if(e->u.MEM->u.BINOP.op == T_plus) {
                    if(e->u.MEM->u.BINOP.right->kind == T_BINOP && 
                        e->u.MEM->u.BINOP.right->u.BINOP.op == T_mul &&
                        e->u.MEM->u.BINOP.right->u.BINOP.right->kind == T_CONST) {
                        Temp_temp r = Temp_newtemp();
                        Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.left);
                        Temp_temp r2 = munchExp(e->u.MEM->u.BINOP.right->u.BINOP.left);
                        sprintf(s, "movq (`s0, `s1, %d), `d0",
                            e->u.MEM->u.BINOP.right->u.BINOP.right->u.CONST);
                        emit(AS_Oper(s, Temp_TempList(r, NULL),
                            Temp_TempList(r1, Temp_TempList(r2, NULL)), NULL));
                        return r;
                    }
                    else if(e->u.MEM->u.BINOP.left->kind == T_CONST) {
                        Temp_temp r = Temp_newtemp();
                        sprintf(s, "movq %d(`s0), `d0", e->u.MEM->u.BINOP.left->u.CONST);
                        emit(AS_Oper(s, Temp_TempList(r, NULL),
                            Temp_TempList(munchExp(e->u.MEM->u.BINOP.right), NULL), NULL));
                        return r;
                    }
                    else if(e->u.MEM->u.BINOP.right->kind == T_CONST) {
                        Temp_temp r = Temp_newtemp();
                        sprintf(s, "movq %d(`s0), `d0", e->u.MEM->u.BINOP.right->u.CONST);
                        emit(AS_Oper(s, Temp_TempList(r, NULL),
                            Temp_TempList(munchExp(e->u.MEM->u.BINOP.left), NULL), NULL));
                        return r;
                    }
                    else {
                        Temp_temp r = Temp_newtemp();
                        emit(AS_Oper("movq (`s0, `s1), `d0", Temp_TempList(r, NULL),
                            Temp_TempList(munchExp(e->u.MEM->u.BINOP.left),
                            Temp_TempList(munchExp(e->u.MEM->u.BINOP.right), NULL)), NULL));
                        return r;
                    }
                }
            }
            else if(e->u.MEM->kind == T_CONST) {
                Temp_temp r = Temp_newtemp();
                sprintf(s, "movq %d, `d0", e->u.MEM->u.CONST);
                emit(AS_Oper(s, Temp_TempList(r, NULL), NULL, NULL));
                return r;
            }
            else {
                Temp_temp r = Temp_newtemp();
                emit(AS_Oper("movq (`s0), `d0", Temp_TempList(r, NULL),
                    Temp_TempList(munchExp(e->u.MEM), NULL), NULL));
                return r;
            }
        }
        case T_BINOP: {
            switch(e->u.BINOP.op) {
                case T_plus: {
                    if(e->u.BINOP.left->kind == T_CONST) {
                        Temp_temp r = Temp_newtemp();
                        sprintf(s, "leaq %d(`s0), `d0", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(s, Temp_TempList(r, NULL),
                            Temp_TempList(munchExp(e->u.BINOP.right), NULL), NULL));
                        return r;
                    }
                    else if(e->u.BINOP.right->kind == T_CONST) {
                        Temp_temp r = Temp_newtemp();
                        sprintf(s, "leaq %d(`s0), `d0", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(s, Temp_TempList(r, NULL),
                            Temp_TempList(munchExp(e->u.BINOP.left), NULL), NULL));
                        return r;
                    }
                    else {
                        Temp_temp r1 = munchExp(e->u.BINOP.left),
                            r2 = munchExp(e->u.BINOP.right);
                        sprintf(s, "addq `s1, `d0");
                        emit(AS_Oper(s, Temp_TempList(r1, NULL),
                            Temp_TempList(r1, Temp_TempList(r2, NULL)), NULL));
                        return r1;
                    }
                }
                case T_minus: {
                    Temp_temp r1 = munchExp(e->u.BINOP.left),
                        r2 = munchExp(e->u.BINOP.right);
                    sprintf(s, "subq `s1, `d0");
                    emit(AS_Oper(s, Temp_TempList(r1, NULL),
                        Temp_TempList(r1, Temp_TempList(r2, NULL)), NULL));
                    return r1;
                }
                case T_mul: {
                    Temp_temp r1 = munchExp(e->u.BINOP.left),
                        r2 = munchExp(e->u.BINOP.right);
                    sprintf(s, "imulq `s1, `d0");
                    emit(AS_Oper(s, Temp_TempList(r1, NULL),
                        Temp_TempList(r1, Temp_TempList(r2, NULL)), NULL));
                    return r1;
                }
                case T_div: {
                    Temp_temp rax = F_RV();
                    Temp_temp rdx = F_DX();
                    Temp_temp r1 = munchExp(e->u.BINOP.left),
                        r2 = munchExp(e->u.BINOP.right);
                    emit(AS_Oper("movq `s0, `d0", Temp_TempList(rax, NULL),
                        Temp_TempList(r1, NULL), NULL));
                    emit(AS_Oper("cqto", Temp_TempList(rax, Temp_TempList(rdx, NULL)),
                        NULL, NULL));
                    emit(AS_Oper("idivq `s0", Temp_TempList(rax, Temp_TempList(rdx, NULL)),
                        Temp_TempList(r2, NULL), NULL));
                    return rax;
                }
            }
        }
        case T_CONST: {
            Temp_temp r = Temp_newtemp();
            sprintf(s, "movq $%d, `d0", e->u.CONST);
            emit(AS_Oper(s, Temp_TempList(r, NULL), NULL, NULL));
            return r;
        }
        case T_TEMP: {
            return e->u.TEMP;
        }
        case T_NAME: {
            Temp_temp r = Temp_newtemp();
            sprintf(s, "movq $%s, `d0", Temp_labelstring(e->u.NAME));
            emit(AS_Oper(s, Temp_TempList(r, NULL), NULL, NULL));
            return r;
        }
        default: {
            fprintf(stderr, "Error!\n");
        }
    }
}

void munchStm(T_stm s) {
    char *str = checked_malloc(40);;
    switch(s->kind) {
        case T_MOVE: {
            if(s->u.MOVE.dst->kind == T_MEM) {
                T_exp e = s->u.MOVE.dst;
                if(e->u.MEM->kind == T_BINOP) {
                    if(e->u.MEM->u.BINOP.op == T_plus) {
                        if(e->u.MEM->u.BINOP.right->kind == T_BINOP && 
                            e->u.MEM->u.BINOP.right->u.BINOP.op == T_mul &&
                            e->u.MEM->u.BINOP.right->u.BINOP.right->kind == T_CONST) {
                            Temp_temp src = munchExp(s->u.MOVE.src);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.left);
                            Temp_temp r2 = munchExp(e->u.MEM->u.BINOP.right->u.BINOP.left);
                            sprintf(str, "movq `s0, (`s1, `s2, %d)",
                                e->u.MEM->u.BINOP.right->u.BINOP.right->u.CONST);
                            emit(AS_Oper(str, NULL, Temp_TempList(src,
                                Temp_TempList(r1, Temp_TempList(r2, NULL))), NULL));
                            return;
                        }
                        else if(e->u.MEM->u.BINOP.left->kind == T_CONST) {
                            Temp_temp src = munchExp(s->u.MOVE.src);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.right);
                            sprintf(str, "movq `s0, %d(`s1)", e->u.MEM->u.BINOP.left->u.CONST);
                            emit(AS_Oper(str, NULL, Temp_TempList(src,
                                Temp_TempList(r1, NULL)), NULL));
                            return;
                        }
                        else if(e->u.MEM->u.BINOP.right->kind == T_CONST) {
                            Temp_temp src = munchExp(s->u.MOVE.src);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.left);
                            sprintf(str, "movq `s0, %d(`s1)", e->u.MEM->u.BINOP.right->u.CONST);
                            emit(AS_Oper(str, NULL, Temp_TempList(src,
                                Temp_TempList(r1, NULL)), NULL));
                            return;
                        }
                        else {
                            Temp_temp src = munchExp(s->u.MOVE.src);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.left);
                            Temp_temp r2 = munchExp(e->u.MEM->u.BINOP.right);
                            Temp_temp r = Temp_newtemp();
                            emit(AS_Oper("movq `s0, (`s1, `s2)", NULL,
                                Temp_TempList(src, Temp_TempList(r1,
                                    Temp_TempList(r2, NULL))), NULL));
                            return;
                        }
                    }
                }
                else if(e->u.MEM->kind == T_CONST) {
                    Temp_temp src = munchExp(s->u.MOVE.src);
                    sprintf(str, "movq `s0, %d", e->u.MEM->u.CONST);
                    emit(AS_Oper(str, NULL, Temp_TempList(src, NULL), NULL));
                    return;
                }
                Temp_temp src = munchExp(s->u.MOVE.src);
                emit(AS_Oper("movq `s0, (`s1)", NULL,
                    Temp_TempList(src, Temp_TempList(munchExp(e->u.MEM), NULL)), NULL));
                return;
            }
            else if(s->u.MOVE.src->kind == T_MEM) {
                T_exp e = s->u.MOVE.src;
                if(e->u.MEM->kind == T_BINOP) {
                    if(e->u.MEM->u.BINOP.op == T_plus) {
                        if(e->u.MEM->u.BINOP.right->kind == T_BINOP && 
                            e->u.MEM->u.BINOP.right->u.BINOP.op == T_mul &&
                            e->u.MEM->u.BINOP.right->u.BINOP.right->kind == T_CONST) {
                            Temp_temp dst = munchExp(s->u.MOVE.dst);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.left);
                            Temp_temp r2 = munchExp(e->u.MEM->u.BINOP.right->u.BINOP.left);
                            sprintf(str, "movq (`s0, `s1, %d), `s2",
                                e->u.MEM->u.BINOP.right->u.BINOP.right->u.CONST);
                            emit(AS_Oper(str, NULL, Temp_TempList(r1,
                                Temp_TempList(r2, Temp_TempList(dst, NULL))), NULL));
                            return;
                        }
                        else if(e->u.MEM->u.BINOP.left->kind == T_CONST) {
                            Temp_temp dst = munchExp(s->u.MOVE.dst);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.right);
                            sprintf(str, "movq %d(`s0), `s1", e->u.MEM->u.BINOP.left->u.CONST);
                            emit(AS_Oper(str, NULL, Temp_TempList(r1,
                                Temp_TempList(dst, NULL)), NULL));
                            return;
                        }
                        else if(e->u.MEM->u.BINOP.right->kind == T_CONST) {
                            Temp_temp dst = munchExp(s->u.MOVE.dst);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.left);
                            sprintf(str, "movq %d(`s0), `s1", e->u.MEM->u.BINOP.right->u.CONST);
                            emit(AS_Oper(str, NULL, Temp_TempList(r1,
                                Temp_TempList(dst, NULL)), NULL));
                            return;
                        }
                        else {
                            Temp_temp dst = munchExp(s->u.MOVE.dst);
                            Temp_temp r1 = munchExp(e->u.MEM->u.BINOP.left);
                            Temp_temp r2 = munchExp(e->u.MEM->u.BINOP.right);
                            Temp_temp r = Temp_newtemp();
                            emit(AS_Oper("movq (`s0, `s1), `s2", NULL,
                                Temp_TempList(r1, Temp_TempList(r2,
                                    Temp_TempList(dst, NULL))), NULL));
                            return;
                        }
                    }
                }
                else if(e->u.MEM->kind == T_CONST) {
                    Temp_temp dst = munchExp(s->u.MOVE.dst);
                    sprintf(str, "movq %d, `s0", e->u.MEM->u.CONST);
                    emit(AS_Oper(str, NULL, Temp_TempList(dst, NULL), NULL));
                    return;
                }
                Temp_temp dst = munchExp(s->u.MOVE.dst);
                emit(AS_Oper("movq (`s0), `s1", NULL,
                    Temp_TempList(munchExp(e->u.MEM), Temp_TempList(dst, NULL)), NULL));
                return;
            }
            else if(s->u.MOVE.src->kind == T_CALL) {
                if(s->u.MOVE.src->u.CALL.fun->kind == T_NAME) {
                    Temp_tempList l = munchArgs(0, s->u.MOVE.src->u.CALL.args);
                    sprintf(str, "callq %s",
                        Temp_labelstring(s->u.MOVE.src->u.CALL.fun->u.NAME));
                    emit(AS_Oper(str, F_calldefs(), l, NULL));
                    emit(AS_Move("movq `s0, `d0", Temp_TempList(munchExp(s->u.MOVE.dst),
                        NULL), Temp_TempList(F_RV(), NULL)));
                    return;
                }
                Temp_temp r = munchExp(s->u.MOVE.src->u.CALL.fun);
                Temp_tempList l = munchArgs(0, s->u.MOVE.src->u.CALL.args);
                emit(AS_Oper("callq `s0", F_calldefs(), Temp_TempList(r, l), NULL));
                emit(AS_Move("movq `s0, `d0", Temp_TempList(munchExp(s->u.MOVE.dst),
                        NULL), Temp_TempList(F_RV(), NULL)));
                return;
            }
            else {
                emit(AS_Move("movq `s0, `d0",
                    Temp_TempList(munchExp(s->u.MOVE.dst), NULL),
                    Temp_TempList(munchExp(s->u.MOVE.src), NULL)));
                return;
            }
        }
        case T_EXP: {
            if(s->u.EXP->kind == T_CALL) {
                if(s->u.EXP->u.CALL.fun->kind == T_NAME) {
                    Temp_tempList l = munchArgs(0, s->u.EXP->u.CALL.args);
                    sprintf(str, "callq %s",
                        Temp_labelstring(s->u.EXP->u.CALL.fun->u.NAME));
                    emit(AS_Oper(str, F_calldefs(), l, NULL));
                    return;
                }
                Temp_temp r = munchExp(s->u.EXP->u.CALL.fun);
                Temp_tempList l = munchArgs(0, s->u.EXP->u.CALL.args);
                emit(AS_Oper("callq `s0", F_calldefs(), Temp_TempList(r, l), NULL));
                return;
            }
            else {
                munchExp(s->u.EXP);
                return;
            }
        }
        case T_CJUMP: {
            Temp_temp r1 = munchExp(s->u.CJUMP.left);
            Temp_temp r2 = munchExp(s->u.CJUMP.right);
            emit(AS_Oper("cmpq `s0, `s1", NULL,
                Temp_TempList(r2, Temp_TempList(r1, NULL)), NULL));
            switch(s->u.CJUMP.op) {
                case T_eq: 
                    sprintf(str, "je %s", Temp_labelstring(s->u.CJUMP.true));
                    break;
                case T_ne:
                    sprintf(str, "jne %s", Temp_labelstring(s->u.CJUMP.true));
                    break;
                case T_ge:
                    sprintf(str, "jge %s", Temp_labelstring(s->u.CJUMP.true));
                    break;
                case T_gt:
                    sprintf(str, "jg %s", Temp_labelstring(s->u.CJUMP.true));
                    break;
                case T_le:
                    sprintf(str, "jle %s", Temp_labelstring(s->u.CJUMP.true));
                    break;
                case T_lt:
                    sprintf(str, "jl %s", Temp_labelstring(s->u.CJUMP.true));
                    break;
                default:
                    sprintf(str, "jmp %s", Temp_labelstring(s->u.CJUMP.true));
                    break;
            }
            
            emit(AS_Oper(str, NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true,
                Temp_LabelList(s->u.CJUMP.false, NULL)))));
            return;
        }
        case T_JUMP: {
            if(s->u.JUMP.exp->kind == T_NAME) {
                sprintf(str, "jmp %s", Temp_labelstring(s->u.JUMP.exp->u.NAME));
                emit(AS_Oper(str, NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
                return;
            }
            Temp_temp r = munchExp(s->u.JUMP.exp);
            emit(AS_Oper("jum `s0", NULL, Temp_TempList(r, NULL),
                AS_Targets(s->u.JUMP.jumps)));
            return;
        }
        case T_LABEL: {
            sprintf(str, "%s", Temp_labelstring(s->u.LABEL));
            emit(AS_Label(str, s->u.LABEL));
            return;
        }
        default: {
            fprintf(stderr, "Error!\n");
        }
    }
}

static Temp_tempList munchArgs(int num, T_expList args) {
    if(args == NULL) return NULL;
    Temp_temp arg = munchExp(args->head);
    Temp_tempList l = munchArgs(num+1, args->tail);
    if(num < 6) {
        Temp_temp dst = F_ARG(num);
        emit(AS_Move("movq `s0, `d0", Temp_TempList(dst, NULL), Temp_TempList(arg, NULL)));
    }
    else {
        emit(AS_Oper("pushq `s0", Temp_TempList(F_SP(), NULL), 
            Temp_TempList(arg, NULL), NULL));
    }
    return Temp_TempList(arg, l);
}
