#pragma once

#define SHM_NAME "/shared_memory_3coloring"
#define SHM_FLAGS O_CREAT | O_RDWR

#define SEM_FREE "/sem_free_3coloring"
#define SEM_USED "/sem_used_3coloring"
#define SEM_MUTEX "/sem_mutex_3coloring"

#define PERM 0600

typedef struct {
    unsigned int vertex1;
    unsigned int vertex2;
} edge_t;

typedef struct {
   edge_t *edges;
   unsigned int num_edges;
   unsigned int num_vertices;
} graph_t;

void free_graph(graph_t *graph);
