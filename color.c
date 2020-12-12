#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "table.h"

G_nodeList simplifyWorklist, freezeWorklist, spillWorklist, spilledNodes, coalescedNodes,
	coloredNodes, selectStack;
Live_moveList coalescedMoves, constrainedMoves, frozenMoves, worklistMoves, activeMoves;
G_table moveList, degree, alias;
Temp_map preColored, color;
string registers[16] = {"%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp",
"%rsp", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};

void init(G_table ml, Temp_map pre, Live_moveList workml) {
	simplifyWorklist = freezeWorklist = spillWorklist = NULL;
	spilledNodes = coalescedNodes = coloredNodes = selectStack = NULL;
	coalescedMoves = constrainedMoves = frozenMoves = activeMoves = NULL;
	worklistMoves = workml;
	moveList = ml;
	degree = G_empty();
	alias = G_empty();
	preColored = pre;
	Temp_map newMap = Temp_empty();
	color = Temp_layerMap(preColored, newMap);
}

bool isPreColored(G_node node) {
	Temp_temp temp = Live_gtemp(node);
	return Temp_look(preColored, temp) != NULL;
}

int getDegree(G_node n) {
	int *d = G_look(degree, n);
	if(d == NULL) return 100;
	return *d;
}

bool moveRelated(G_node node) {
	Live_moveList ml = G_look(moveList, node);
	return ml != NULL;
}

Live_moveList nodeMoves(G_node n) {
	Live_moveList ml = G_look(moveList, n);
	return Live_moveIntersection(ml, Live_moveUnion(activeMoves, worklistMoves));
}

G_node getAlias(G_node n) {
	if(G_inNodeList(n, coalescedNodes))
		return (G_node)G_look(alias, n);
	else
		return n;
}

void makeWorklist(G_graph ig) {
	for(G_nodeList unHandled = G_nodes(ig); unHandled; unHandled = unHandled->tail) {
		G_node node = unHandled->head;
		if(isPreColored(node)) continue;
		int *d = checked_malloc(sizeof(d));
		*d = G_degree(node);
		G_enter(degree, node, d);
		if(G_degree(node) >= 16) {
			spillWorklist = G_NodeList(node, spillWorklist);
		}
		else if(moveRelated(node)) {
			freezeWorklist = G_NodeList(node, freezeWorklist);
		}
		else
			simplifyWorklist = G_NodeList(node, simplifyWorklist);
	}
}

bool valid(G_node n) {
	if(G_inNodeList(n, selectStack) || G_inNodeList(n, coalescedNodes))
		return FALSE;
	return TRUE;
}

void enableMoves(G_nodeList nl) {
	for(; nl; nl = nl->tail) {
		if(valid(nl->head)) {
			for(Live_moveList ml = nodeMoves(nl->head); ml; ml = ml->tail) {
				Live_move move = ml->head;
				if(Live_moveIn(move, activeMoves)) {
					activeMoves = Live_moveRemove(move, activeMoves);
					worklistMoves = Live_MoveAppend(move, worklistMoves);
				}
			}
		}
	}
}

void decrementDegree(G_node n) {
	if(isPreColored(n)) return;
	int *d = G_look(degree, n);
	*d = *d - 1;
	if(*d == 15) {
		enableMoves(G_NodeList(n, G_adj(n)));
		spillWorklist = G_removeFromList(n, spillWorklist);
		if(moveRelated(n))
			freezeWorklist = G_NodeList(n, freezeWorklist);
		else
			simplifyWorklist = G_NodeList(n, simplifyWorklist);
	}
}

void simplify() {
	G_node n = simplifyWorklist->head;
	simplifyWorklist = simplifyWorklist->tail;
	selectStack = G_NodeList(n, selectStack);
	for(G_nodeList adj = G_adj(n); adj; adj = adj->tail)
		if(valid(adj->head)) decrementDegree(adj->head);
}

void addWorklist(G_node n) {
	if(!isPreColored(n) && !moveRelated(n) && getDegree(n) < 16) {
		freezeWorklist = G_removeFromList(n, freezeWorklist);
		simplifyWorklist = G_NodeList(n, simplifyWorklist);
	}
}

bool OK(G_node t, G_node u) {
	return getDegree(t) < 16 || isPreColored(t) || G_inNodeList(t, G_adj(u));
}

bool isOK(G_node v, G_node u) {
	for(G_nodeList nl = G_adj(v); nl; nl = nl->tail) {
		if(valid(nl->head)) {
			if(!OK(nl->head, u)) return FALSE;
		}
	}
	return TRUE;
}

bool conservative(G_node u, G_node v) {
	G_nodeList uadj = G_adj(u), vadj = G_adj(v);
	int k = 0;
	for(; uadj; uadj = uadj->tail) {
		G_node n = uadj->head;
		if(G_inNodeList(n, vadj)) continue;
		if(valid(n) && getDegree(n) >= 16) {
			k++;
		}
	}
	for(; vadj; vadj = vadj->tail) {
		G_node n = vadj->head;
		if(valid(n) && getDegree(n) >= 16) {
			k++;
		}
	}
	return k < 16;
}

void combine(G_node u, G_node v) {
	if(G_inNodeList(v, freezeWorklist))
		freezeWorklist = G_removeFromList(v, freezeWorklist);
	else
		spillWorklist = G_removeFromList(v, spillWorklist);
	coalescedNodes = G_NodeList(v, coalescedNodes);
	G_enter(alias, v, u);
	Live_moveList uMoveList = G_look(moveList, u), vMoveList = G_look(moveList, v);
	Live_moveList newMoveList = Live_moveUnion(uMoveList, vMoveList);
	G_enter(moveList, u, newMoveList);
	enableMoves(G_NodeList(v, NULL));
	for(G_nodeList vadj = G_adj(v); vadj; vadj = vadj->tail) {
		G_node n = vadj->head;
		if(valid(n)) {
			G_addEdge(n, u);
			if(!isPreColored(u)) {
				int *d = G_look(degree, u);
				*d = *d + 1;
			}
			if(!isPreColored(n)) {
				int *d = G_look(degree, n);
				*d = *d + 1;
			}
			decrementDegree(n);
		}
	}
	if(getDegree(u) >= 16 && G_inNodeList(u, freezeWorklist)) {
		freezeWorklist = G_removeFromList(u, freezeWorklist);
		spillWorklist = G_NodeList(u, spillWorklist);
	} 
}

void coalesce() {
	Live_move move = worklistMoves->head;
	G_node x = getAlias(move->src);
	G_node y = getAlias(move->dst);
	G_node u, v;
	if(isPreColored(y)) {
		u = y; v = x;
	}
	else {
		u = x; v = y;
	}
	worklistMoves = worklistMoves->tail;
	if(v == u) {
		coalescedMoves = Live_MoveAppend(move, coalescedMoves);
		addWorklist(u);
	}
	else if(isPreColored(v) || G_inNodeList(u, G_adj(v))) {
		constrainedMoves = Live_MoveAppend(move, constrainedMoves);
		addWorklist(u); addWorklist(v);
	}
	else if((isPreColored(u) && isOK(v, u)) || (!isPreColored(u) && conservative(u, v))) {
		coalescedMoves = Live_MoveAppend(move, coalescedMoves);
		combine(u, v);
		addWorklist(u);
	}
	else {
		activeMoves = Live_MoveAppend(move, activeMoves);
	}
}

void freezeMoves(G_node u) {
	for(Live_moveList ml = nodeMoves(u); ml; ml = ml->tail) {
		Live_move move = ml->head;
		G_node v;
		if(getAlias(move->dst) == getAlias(u))
			v = getAlias(move->src);
		else
			v = getAlias(move->dst);
		activeMoves = Live_moveRemove(move, activeMoves);
		frozenMoves = Live_MoveAppend(move, frozenMoves);
		if(nodeMoves(v) == NULL && !isPreColored(v) && getDegree(v) < 16) {
			freezeWorklist = G_removeFromList(v, freezeWorklist);
			simplifyWorklist = G_NodeList(v, simplifyWorklist);
		}
	}
}

void freeze() {
	G_node n = freezeWorklist->head;
	freezeWorklist = freezeWorklist->tail;
	simplifyWorklist = G_NodeList(n, simplifyWorklist);
	freezeMoves(n);
}

void selectSpill() {
	G_node n = spillWorklist->head;
	spillWorklist = spillWorklist->tail;
	simplifyWorklist = G_NodeList(n, simplifyWorklist);
	freezeMoves(n);
}

void assignColors() {
	for(; selectStack; selectStack = selectStack->tail) {
		G_node n = selectStack->head;
		bool okColors[16];
		for(int i = 0; i < 16; i++)
			okColors[i] = TRUE;
		for(G_nodeList nl = G_adj(n); nl; nl = nl->tail) {
			G_node adj = getAlias(nl->head);
			if(G_inNodeList(adj, coloredNodes) || isPreColored(adj)) {
				string c = Temp_look(color, Live_gtemp(adj));
				for(int i = 0; i < 16; i++) {
					if(strcmp(c, registers[i]) == 0) {
						okColors[i] = FALSE;
						break;
					}
				}
			}
		}
		int ok = 0;
		for(; ok < 16; ok++) {
			if(okColors[ok] == TRUE) break;
		}
		if(ok >= 16) {
			spilledNodes = G_NodeList(n, spilledNodes);
		}
		else {
			coloredNodes = G_NodeList(n, coloredNodes);
			Temp_enter(color, Live_gtemp(n), registers[ok]);
		}
	}
	for(G_nodeList nl = coalescedNodes; nl; nl = nl->tail) {
		G_node n = nl->head;
		string c = Temp_look(color, Live_gtemp(getAlias(n)));
		Temp_enter(color, Live_gtemp(n), c);
	}
}

Temp_tempList makeSpilledList(G_nodeList nl) {
	Temp_tempList tl = NULL;
	for(; nl; nl = nl->tail)
		tl = Temp_TempList(Live_gtemp(nl->head), tl);
	return tl;
}

struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs,
	Live_moveList moves, G_table tempToMove) {
	//your code here.
	init(tempToMove, initial, moves);
	makeWorklist(ig);
	do {
		if(simplifyWorklist) simplify();
		else if(worklistMoves) coalesce();
		else if(freezeWorklist) freeze();
		else if(spillWorklist) selectSpill();
	} while(simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist);

	assignColors();
	struct COL_result ret;
	ret.coloring = color;
	ret.spills = makeSpilledList(spilledNodes);
	return ret;
}
