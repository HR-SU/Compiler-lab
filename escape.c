#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"
#include "table.h"

typedef struct escapeEntry_ *escapeEntry;
struct escapeEntry_ {
	int depth;
	bool *escape;
};

escapeEntry EscapeEntry(int d, bool *escape) {
	escapeEntry e = checked_malloc(sizeof(*e));
	e->depth = d;
	e->escape = escape;
	return e;
}

static void traverseExp(S_table env, int depth, A_exp e);
static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);

void traverseExp(S_table env, int depth, A_exp e) {
	if(e == NULL) return;
	switch(e->kind) {
		case A_varExp: {
			traverseVar(env, depth, e->u.var);
			return;
		}
		case A_nilExp:
		case A_intExp:
		case A_stringExp: {
			return;
		}
		case A_callExp: {
			for(A_expList args = e->u.call.args; args; args = args->tail)
				traverseExp(env, depth, args->head);
			return;
		}
		case A_opExp: {
			traverseExp(env, depth, e->u.op.left);
			traverseExp(env, depth, e->u.op.right);
			return;
		}
		case A_recordExp: {
			for(A_efieldList fields = e->u.record.fields; fields; fields = fields->tail)
				traverseExp(env, depth, fields->head->exp);
			return;
		}
		case A_seqExp: {
			for(A_expList exp = e->u.seq; exp; exp = exp->tail)
				traverseExp(env, depth, exp->head);
			return;
		}
		case A_assignExp: {
			traverseExp(env, depth, e->u.assign.exp);
			traverseVar(env, depth, e->u.assign.var);
			return;
		}
		case A_ifExp: {
			traverseExp(env, depth, e->u.iff.test);
			traverseExp(env, depth, e->u.iff.elsee);
			traverseExp(env, depth, e->u.iff.then);
			return;
		}
		case A_whileExp: {
			traverseExp(env, depth, e->u.whilee.test);
			traverseExp(env, depth, e->u.whilee.body);
			return;
		}
		case A_forExp: {
			traverseExp(env, depth, e->u.forr.lo);
			traverseExp(env, depth, e->u.forr.hi);
			S_beginScope(env);
			e->u.forr.escape = FALSE;
			S_enter(env, e->u.forr.var, EscapeEntry(depth, &(e->u.forr.escape)));
			traverseExp(env, depth, e->u.forr.body);
			S_endScope(env);
			return;
		}
		case A_breakExp: {
			return;
		}
		case A_letExp: {
			S_beginScope(env);
			for(A_decList decs = e->u.let.decs; decs; decs = decs->tail)
				traverseDec(env, depth, decs->head);
			traverseExp(env, depth, e->u.let.body);
			S_endScope(env);
			return;
		}
		case A_arrayExp: {
			traverseExp(env, depth, e->u.array.init);
			traverseExp(env, depth, e->u.array.size);
			return;
		}
	}
}

void traverseVar(S_table env, int depth, A_var v) {
	if(v == NULL) return;
	switch(v->kind) {
		case A_simpleVar: {
			escapeEntry entry = S_look(env, v->u.simple);
			if(entry &&entry->depth < depth) 
				*(bool *)(entry->escape) = TRUE;
			return;
		}
		case A_fieldVar: {
			traverseVar(env, depth, v->u.field.var);
			return;
		}
		case A_subscriptVar: {
			traverseVar(env, depth, v->u.subscript.var);
			traverseExp(env, depth, v->u.subscript.exp);
			return;
		}
	}
}

static void traverseDec(S_table env, int depth, A_dec d) {
	if(d == NULL) return;
	switch(d->kind) {
		case A_varDec: {
			traverseExp(env, depth, d->u.var.init);
			d->u.var.escape = FALSE;
			S_enter(env, d->u.var.var, EscapeEntry(depth, &(d->u.var.escape)));
			return;
		}
		case A_functionDec: {
			for(A_fundecList fl = d->u.function; fl; fl = fl->tail) {
				S_beginScope(env);
				for(A_fieldList l = fl->head->params; l; l = l->tail) {
					l->head->escape = FALSE;
					S_enter(env, l->head->name, EscapeEntry(depth+1, &(l->head->escape)));
				}
				traverseExp(env, depth+1, fl->head->body);
				S_endScope(env);
			}
			return;
		}
		case A_typeDec: {
			return;
		}
	}
}

void Esc_findEscape(A_exp exp) {
	//your code here
	S_table env = S_empty();
	traverseExp(env, 0, exp);
}
