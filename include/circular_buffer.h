#pragma once

#include <semaphore.h>

#define BUFFER_SIZE 20

#define CIRC_BUF_SUCCESS 0
#define CIRC_BUF_ERR_SEM_WAIT -1
#define CIRC_BUF_ERR_SEM_POST -2

extern int circ_buf_error;
extern sem_t *free_slots, *used_slots, *mutex;

struct circ_buf {
  int buffer[BUFFER_SIZE];
  unsigned int write_pos;
  unsigned int read_pos;
};

void circ_buf_write(struct circ_buf *cb, int val);

int circ_buf_read(struct circ_buf *cb);
