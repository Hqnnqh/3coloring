#include "../include/circular_buffer.h"
#include <semaphore.h>
#include <stdio.h>

int circ_buf_error = CIRC_BUF_SUCCESS;
sem_t *free_slots, *used_slots, *mutex;

void circ_buf_write(struct circ_buf *cb, solution_t val) {
    if (!free_slots || !mutex || sem_wait(free_slots) == -1 || sem_wait(mutex) == -1) {
        circ_buf_error = CIRC_BUF_ERR_SEM_WAIT;
        return;
    }

    cb->buffer[cb->write_pos] = val;

    if (!used_slots || sem_post(mutex) == -1 || sem_post(used_slots) == -1) {
        circ_buf_error = CIRC_BUF_ERR_SEM_POST;
        return;
    }

    cb->write_pos += 1;
    cb->write_pos %= BUFFER_SIZE;
}

// Attempt to read new solution. Returns solution with num_edges = -1 on error and sets error.
solution_t circ_buf_read(struct circ_buf *cb) {
    if (!used_slots || sem_wait(used_slots)) {
        circ_buf_error = CIRC_BUF_ERR_SEM_WAIT;
        solution_t invalid;
        invalid.num_deges = -1;
        return invalid;
    }

    solution_t val = cb->buffer[cb->read_pos];

    if (!used_slots || sem_post(free_slots)) {
        circ_buf_error = CIRC_BUF_ERR_SEM_POST;
        solution_t invalid;
        invalid.num_deges = -1;
        return invalid;
    }

    cb->read_pos += 1;
    cb->read_pos %= BUFFER_SIZE;

    return val;
}
