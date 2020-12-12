#ifndef LIVENESS_H
#define LIVENESS_H

typedef struct Live_move_ *Live_move;
struct Live_move_ {
	G_node src, dst;
};

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	Live_move head;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);
Live_moveList Live_MoveAppend(Live_move head, Live_moveList tail);

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
	G_table tempToMove;
};
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

Live_moveList Live_moveUnion(Live_moveList a, Live_moveList b);
Live_moveList Live_moveIntersection(Live_moveList a, Live_moveList b);
bool Live_moveIn(Live_move a, Live_moveList b);
Live_moveList Live_moveRemove(Live_move a, Live_moveList b);

#endif
