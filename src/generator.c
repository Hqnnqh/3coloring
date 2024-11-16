#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#include "../include/circular_buffer.h"
#include "../include/utils.h"

int main() {
    printf("Hello Generator!\n");

    // dummy implementation for testing
    int fd;

    // create shared memory
    if ((fd = shm_open(SHM_NAME, SHM_FLAGS, PERM)) == -1) {
      perror("shm_open");
      return EXIT_FAILURE;
    }

    // set size of shared memory
    if (ftruncate(fd, sizeof(struct circ_buf)) == -1) {
      perror("ftruncate");
      shm_unlink(SHM_NAME);
      close(fd);
      return EXIT_FAILURE;
    }

    // map shared memory
    struct circ_buf *circ_buf_ptr;
    circ_buf_ptr = mmap(NULL, sizeof(struct circ_buf), PROT_READ | PROT_WRITE, MAP_SHARED,
                  fd, 0);

    if (circ_buf_ptr == MAP_FAILED) {
      perror("mmap");
      shm_unlink(SHM_NAME);
      close(fd);
      return EXIT_FAILURE;
    }

    // set up semaphores

    sem_t *sem_free, *sem_used, *sem_mutex;
    if ((sem_free = sem_open(SEM_FREE, O_CREAT, PERM, BUFFER_SIZE)) ==
        SEM_FAILED) {
      perror("sem_open sem_free");
      shm_unlink(SHM_NAME);
      close(fd);
      return EXIT_FAILURE;
    }

    if ((sem_used = sem_open(SEM_USED, O_CREAT, PERM, 0)) == SEM_FAILED) {
      perror("sem_open sem_used");
      sem_close(sem_free);
      sem_unlink(SEM_FREE);
      shm_unlink(SHM_NAME);
      close(fd);
      return EXIT_FAILURE;
    }

    if ((sem_mutex = sem_open(SEM_MUTEX, O_CREAT, PERM, 1)) == SEM_FAILED) {
      perror("sem_open sem_mutex");
      sem_close(sem_free);
      sem_unlink(SEM_FREE);

      sem_close(sem_used);
      sem_unlink(SEM_USED);

      shm_unlink(SHM_NAME);
      close(fd);
      return EXIT_FAILURE;
    }

    // initialize circular buffer
    used_slots = sem_used;
    free_slots = sem_free;
    mutex = sem_mutex;
    circ_buf_ptr->read_pos = 0;
    circ_buf_ptr->write_pos = 0;

    circ_buf_write(circ_buf_ptr, 3);

    // clean up

    // close sem_free
    if (sem_close(sem_free) == -1) {
      perror("sem_close sem_free");
      sem_close(sem_used);
      sem_close(sem_mutex);
      close(fd);
      return EXIT_FAILURE;
    }

    // close sem_used
    if (sem_close(sem_used) == -1) {
      perror("sem_close sem_used");
      sem_close(sem_mutex);
      close(fd);
      return EXIT_FAILURE;
    }

    // close sem_mutex
    if (sem_close(sem_mutex) == -1) {
      perror("sem_close sem_mutex");
      close(fd);
      return EXIT_FAILURE;
    }

    // unmap shm
    if (munmap(circ_buf_ptr, sizeof(struct circ_buf)) == -1) {
      perror("munmap");
      close(fd);
      return EXIT_FAILURE;
    }

    // close fd
    if (close(fd) == -1) {
      perror("close");
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
