#include <stdio.h>
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
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"

/* void showNode(void *info) {
	Temp_map F_tempMap = F_TempMap();
	Temp_map map = Temp_layerMap(F_tempMap, Temp_name());
	Temp_temp temp = (Temp_temp)info;
	printf("%s:\n", Temp_look(map, temp));
} */

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here
	G_graph fg = FG_AssemFlowGraph(il, f);
	struct Live_graph lg = Live_liveness(fg);
	F_tempMap = F_TempMap();
	// G_show(stdout, G_nodes(lg.graph), showNode);
	struct COL_result result = COL_color(lg.graph, F_tempMap, F_registers(),
		lg.moves, lg.tempToMove);
	struct RA_result ret;
	if(result.spills != NULL) {
		il = AS_rewrite(il, result.coloring);
		AS_instrList ilNew = AS_rewriteSpill(f, il, result.spills);
		return RA_regAlloc(f, ilNew);
	}
	else {
		ret.coloring = result.coloring;
		ret.il = AS_rewrite(il, result.coloring);
	}
	// AS_printInstrList(stdout, il, ret.coloring);
	return ret;
}
