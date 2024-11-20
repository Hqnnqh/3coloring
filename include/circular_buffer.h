#pragma once

#include <semaphore.h>
#include <stdbool.h>

#define MAX_EDGES 8

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
    edge_t edges[MAX_EDGES];
    int num_deges;
} solution_t;

#define BUFFER_SIZE 20

#define CIRC_BUF_SUCCESS 0
#define CIRC_BUF_ERR_SEM_WAIT -1
#define CIRC_BUF_ERR_SEM_POST -2

extern int circ_buf_error;
extern sem_t *free_slots, *used_slots, *mutex;

struct circ_buf {
  solution_t buffer[BUFFER_SIZE];
  unsigned int write_pos;
  unsigned int read_pos;
};

typedef struct {
    bool terminate;
    struct circ_buf buffer;
} shm_t;

vertex_t get_random_vertex(void);

void circ_buf_write(struct circ_buf *cb, solution_t val);

solution_t circ_buf_read(struct circ_buf *cb);
