#pragma once

#define SHM_NAME "/shared_memory_3coloring"
#define SHM_FLAGS O_CREAT | O_RDWR

#define SEM_FREE "/sem_free_3coloring"
#define SEM_USED "/sem_used_3coloring"
#define SEM_MUTEX "/sem_mutex_3coloring"

#define PERM 0600

typedef enum {
    RED,
    BLUE,
    GREEN,
} vertex_t;

typedef struct {
    unsigned int vertex1_index;
    unsigned int vertex2_index;
} edge_t;

typedef struct {
   vertex_t *vertices;
   edge_t *edges;
   unsigned int num_edges;
   unsigned int num_vertices;

} graph_t;

void free_graph(graph_t *graph);

vertex_t get_random_vertex(void);
