#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

Temp_tempList FG_def(G_node n) {
	//your code here.
	AS_instr instr = G_nodeInfo(n);
	if(instr->kind == I_OPER)
		return instr->u.OPER.dst;
	else if(instr->kind == I_MOVE)
		return instr->u.MOVE.dst;
	else
		return NULL;
}

Temp_tempList FG_use(G_node n) {
	//your code here.
	AS_instr instr = G_nodeInfo(n);
	if(instr->kind == I_OPER)
		return instr->u.OPER.src;
	else if(instr->kind == I_MOVE)
		return instr->u.MOVE.src;
	else
		return NULL;
}

bool FG_isMove(G_node n) {
	//your code here.
	AS_instr instr = G_nodeInfo(n);
	return instr->kind == I_MOVE;
}

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
	//your code here.
	G_graph flowGraph = G_Graph();

	bool isSeq = TRUE;
	G_node oldNode = NULL, newNode = NULL;
	AS_instr oldIns = NULL;
	for(AS_instrList ilist = il; ilist; ilist = ilist->tail) {
		newNode = G_Node(flowGraph, ilist->head);
		if(oldNode) {
			if(oldIns->kind == I_OPER && oldIns->u.OPER.jumps)
				isSeq = FALSE;
			else
				isSeq = TRUE;
			if(isSeq == TRUE) {
				G_addEdge(oldNode, newNode);
			}
		}
		oldIns = ilist->head;
		oldNode = newNode;
	}
	
	for(G_nodeList nl = G_nodes(flowGraph); nl; nl = nl->tail) {
		AS_instr instr = G_nodeInfo(nl->head);
		if(instr->kind == I_OPER && instr->u.OPER.jumps) {
			for(Temp_labelList labels = instr->u.OPER.jumps->labels; labels;
				labels = labels->tail) {
				for(G_nodeList nodes = G_nodes(flowGraph); nodes; nodes = nodes->tail) {
					AS_instr ins = G_nodeInfo(nodes->head);
					if(ins->kind == I_LABEL && ins->u.LABEL.label == labels->head) {
						G_addEdge(nl->head, nodes->head);
						break;
					}
				}
			}
		}
	}
	return flowGraph;
}
