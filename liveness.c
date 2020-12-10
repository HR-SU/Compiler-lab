#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}

Temp_tempList tempUnion(Temp_tempList l1, Temp_tempList l2) {
    Temp_tempList result = NULL, last = NULL;
    Temp_temp next;
    
    while(l1 && l2) {
        if(Temp_int(l1->head) < Temp_int(l2->head)) {
            next = l1->head;
            l1 = l1->tail;
        }
        else if(Temp_int(l1->head) > Temp_int(l2->head)) {
            next = l2->head;
            l2 = l2->tail;
        }
        else {
            next = l1->head;
            l1 = l1->tail; l2 = l2->tail;
        }
        if(result == NULL) last = result = Temp_TempList(next, NULL);
        else last = last->tail = Temp_TempList(next, NULL);
    }
    while(l1) {
        next = l1->head;
        if(result == NULL) last = result = Temp_TempList(next, NULL);
        else last = last->tail = Temp_TempList(next, NULL);
        l1 = l1->tail;
    }
    while(l2) {
        next = l2->head;
        if(result == NULL) last = result = Temp_TempList(next, NULL);
        else last = last->tail = Temp_TempList(next, NULL);
        l2 = l2->tail;
    }
    return result;
}

Temp_tempList tempDiff(Temp_tempList l1, Temp_tempList l2) {
    Temp_tempList result = NULL, last = NULL;
    while(l1 && l2) {
        if(Temp_int(l1->head) < Temp_int(l2->head)) {
            if(result == NULL) last = result = Temp_TempList(l1->head, NULL);
            else last = last->tail = Temp_TempList(l1->head, NULL);
            l1 = l1->tail;
        }
        else if(Temp_int(l1->head) > Temp_int(l2->head)) {
            l2 = l2->tail;
        }
        else {
            l1 = l1->tail;
            l2 = l2->tail;
        }
    }
    while(l1) {
        if(result == NULL) last = result = Temp_TempList(l1->head, NULL);
        else last = last->tail = Temp_TempList(l1->head, NULL);
        l1 = l1->tail;
    }
    return result;
}

Temp_tempList tempReorder(Temp_tempList list) {
	if(list == NULL) return NULL;
	Temp_tempList result = NULL;
	for(; list; list = list->tail) {
		if(result == NULL)
			result = Temp_TempList(list->head, NULL);
		else {
			if(Temp_int(result->head) > Temp_int(list->head)) {
				result = Temp_TempList(list->head, result);
				continue;
			}
			Temp_tempList tmp = result;
			while(tmp->tail && Temp_int(tmp->tail->head) < Temp_int(list->head))
				tmp = tmp->tail;
			if(Temp_int(tmp->head) == Temp_int(list->head)) continue;
			tmp->tail = Temp_TempList(list->head, tmp->tail);
		}
	}
	return result;
}

void tempFree(Temp_tempList list) {
    while(list) {
        Temp_tempList l = list;
        list = list->tail;
        free(l);
    }
}

G_node findNodeByTemp(G_graph g, Temp_temp temp) {
	for(G_nodeList nodes = G_nodes(g); nodes; nodes = nodes->tail)
		if(G_nodeInfo(nodes->head) == temp) return nodes->head;
	return G_Node(g, temp);
}

Temp_temp Live_gtemp(G_node n) {
	//your code here.
	return G_nodeInfo(n);
}

struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	struct Live_graph lg;
	G_table inTable = G_empty(), outTable = G_empty(), defTable = G_empty();
	for(G_nodeList nodes = G_nodes(flow); nodes; nodes = nodes->tail) {
		Temp_tempList use = FG_use(nodes->head);
		G_enter(inTable, nodes->head, tempReorder(use));
		Temp_tempList def = FG_def(nodes->head);
		G_enter(defTable, nodes->head, tempReorder(def));
		G_enter(outTable, nodes->head, NULL);
	}
	bool flag = TRUE;
	while(flag) {
		flag = FALSE;
		for(G_nodeList nodes = G_nodes(flow); nodes; nodes = nodes->tail) {
			Temp_tempList in = G_look(inTable, nodes->head);
			Temp_tempList out = G_look(outTable, nodes->head);
			Temp_tempList def = G_look(defTable, nodes->head);
			Temp_tempList diff = tempDiff(out, def);
			Temp_tempList newIn = tempUnion(in, diff);
			G_nodeList succs = G_succ(nodes->head);
			Temp_tempList newOut = NULL;
			for(; succs; succs = succs->tail) {
				Temp_tempList tmp = newOut;
				newOut = tempUnion(tmp, G_look(inTable, succs->head));
				tempFree(tmp);
			}
			Temp_tempList l1=tempDiff(newOut, out), l2=tempDiff(newIn, in);
			if(l1 || l2) {
				flag = TRUE;
				tempFree(l1); tempFree(l2);
			}
			G_enter(inTable, nodes->head, newIn);
			G_enter(outTable, nodes->head, newOut);
			tempFree(diff); tempFree(in); tempFree(out);
		}
	}

	G_graph conflict = G_Graph();
	Live_moveList moveList = NULL;
	for(G_nodeList nodes = G_nodes(flow); nodes; nodes = nodes->tail) {
		Temp_tempList def = FG_def(nodes->head);
		Temp_tempList out = G_look(outTable, nodes->head);
		for(; def; def = def->tail) {
			G_node d = findNodeByTemp(conflict, def);
			if(FG_isMove(nodes->head)) {
				G_node s = findNodeByTemp(conflict, FG_use(nodes->head)->head);
				moveList = Live_MoveList(s, d, moveList);
			}
			for(; out; out = out->tail) {
				G_node o = findNodeByTemp(conflict, out);
				if(FG_isMove(nodes->head)) {
					if(FG_use(nodes->head)->head == out) continue;
				}
				if(G_goesTo(d, o) || G_goesTo(o, d)) continue;
				else G_addEdge(d, o);
			}
		}
	}

	lg.graph = conflict;
	lg.moves = moveList;
	return lg;
}


