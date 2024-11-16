#include "../include/circular_buffer.h"
#include <semaphore.h>

int circ_buf_error = CIRC_BUF_SUCCESS;
sem_t *free_slots, *used_slots, *mutex;


void circ_buf_write(struct circ_buf *cb, int val) {
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

int circ_buf_read(struct circ_buf *cb) {
    if (!used_slots || sem_wait(used_slots)) {
        circ_buf_error = CIRC_BUF_ERR_SEM_WAIT;
        return -1;
    }

    int val = cb->buffer[cb->read_pos];

    if (!used_slots || sem_post(free_slots)) {
        circ_buf_error = CIRC_BUF_ERR_SEM_POST;
        return -1;
    }

    cb->read_pos += 1;
    cb->read_pos %= BUFFER_SIZE;

    return val;
}
