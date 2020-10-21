#include "prog1.h"
#include <string.h>
#include <stdio.h>

typedef struct table *Table_;
typedef struct intAndTable *IntAndTable_;
struct table {string id; int value; Table_ tail;};
struct intAndTable {int value; Table_ tablePtr;};

Table_ Table(string id, int value, Table_ tail) {
	Table_ t = checked_malloc(sizeof(*t));
	t->id = id; t->value = value; t->tail = tail;
	return t;
}

IntAndTable_ IntAndTable(int value, Table_ tablePtr) {
	IntAndTable_ t = checked_malloc(sizeof(*t));
	t->value = value; t->tablePtr = tablePtr;
	return t;
}

int maxargs_exp(A_exp exp);
Table_ interpStm(A_stm stm, Table_ t);
IntAndTable_ interpExp(A_exp exp, Table_ t);
int lookUp(string id, Table_ t);

int maxargs(A_stm stm)
{
	int max = 0;
	switch (stm->kind)
	{
	case A_compoundStm: {
		int max1 = maxargs(stm->u.compound.stm1);
		int max2 = maxargs(stm->u.compound.stm2);
		int max3 = max1 > max2 ? max1 : max2;
		max = max3 > max ? max3 : max;
		break;
	}
	case A_assignStm: {
		int max1 = maxargs_exp(stm->u.assign.exp);
		max = max1 > max ? max1 : max;
		break;
	}
	case A_printStm: {
		A_expList exps = stm->u.print.exps;
		int length = 0;
		while(exps->kind != A_lastExpList) {
			assert(exps->kind == A_pairExpList);
			int max1 = maxargs_exp(exps->u.pair.head);
			max = max1 > max ? max1 : max;
			length++;
			exps = exps->u.pair.tail;
		}
		int max1 = maxargs_exp(exps->u.last);
		max = max1 > max ? max1 : max;
		length++;
		max = length > max ? length : max;
		break;
	}
	
	default:
		break;
	}

	return max;
}

int maxargs_exp(A_exp exp) {
	int max = 0;
	switch (exp->kind)
	{
	case A_idExp:
	case A_numExp:
		break;
	case A_opExp: {
		int max1 = maxargs_exp(exp->u.op.left);
		int max2 = maxargs_exp(exp->u.op.right);
		int max3 = max1 > max2 ? max1 : max2;
		max = max3 > max ? max3 : max;
		break;
	}
	case A_eseqExp: {
		int max1 = maxargs(exp->u.eseq.stm);
		int max2 = maxargs_exp(exp->u.eseq.exp);
		int max3 = max1 > max2 ? max1 : max2;
		max = max3 > max ? max3 : max;
		break;
	}
	
	default:
		break;
	}
	return max;
}

void interp(A_stm stm)
{
	interpStm(stm, 0);
	//TODO: put your code here.
}

Table_ interpStm(A_stm stm, Table_ t) {
	switch (stm->kind)
	{
	case A_compoundStm: {
		return 
			interpStm(stm->u.compound.stm2, interpStm(stm->u.compound.stm1, t));
	}
	case A_assignStm: {
		IntAndTable_ intAndt = interpExp(stm->u.assign.exp, t);
		return Table(stm->u.assign.id, intAndt->value, intAndt->tablePtr);
	}
	case A_printStm: {
		A_expList exps = stm->u.print.exps;
		while(exps->kind != A_lastExpList) {
			assert(exps->kind == A_pairExpList);
			IntAndTable_ intAndt = interpExp(exps->u.pair.head, t);
			printf("%d ", intAndt->value);
			t = intAndt->tablePtr;
			exps = exps->u.pair.tail;
		}
		IntAndTable_ intAndt = interpExp(exps->u.last, t);
		printf("%d\n", intAndt->value);
		return intAndt->tablePtr;
	}
	
	default:
		break;
	}
}

IntAndTable_ interpExp(A_exp exp, Table_ t) {
	int max = 0;
	switch (exp->kind)
	{
	case A_idExp: {
		return IntAndTable(lookUp(exp->u.id, t), t);
	}
	case A_numExp: {
		return IntAndTable(exp->u.num, t);
	}
	case A_opExp: {
		IntAndTable_ intAndtl = interpExp(exp->u.op.left, t);
		IntAndTable_ intAndtr = interpExp(exp->u.op.right, intAndtl->tablePtr);
		switch (exp->u.op.oper)
		{
		case A_plus:
			return IntAndTable(intAndtl->value + intAndtr->value, intAndtr->tablePtr);
		case A_minus: 
			return IntAndTable(intAndtl->value - intAndtr->value, intAndtr->tablePtr);
		case A_times:
			return IntAndTable(intAndtl->value * intAndtr->value, intAndtr->tablePtr);
		case A_div: 
			return IntAndTable(intAndtl->value / intAndtr->value, intAndtr->tablePtr);
		
		default:
			break;
		}
	}
	case A_eseqExp: {
		return interpExp(exp->u.eseq.exp, interpStm(exp->u.eseq.stm, t));
	}
	
	default:
		break;
	}
}

int lookUp(string id, Table_ t) {
	if(!strcmp(id, t->id)) {
		return t->value;
	}
	return lookUp(id, t->tail);
}