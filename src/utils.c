#include "../include/utils.h"
#include <stdlib.h>

void free_graph(graph_t* graph) {
    free(graph->edges);
}
