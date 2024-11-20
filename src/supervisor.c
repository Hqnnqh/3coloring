#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#include "../include/circular_buffer.h"
#include "../include/utils.h"

volatile sig_atomic_t quit = 0;

void handle_signal(int signal) { quit = 1; }

void usage_exit(void) {
  printf("supervisor [-n limit] [-w delay]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  printf("Hello Supervisor!\n");

  struct sigaction sa = {.sa_handler = handle_signal};
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  // limit is infinite if it remains a NULL pointer
  int *limit = NULL, delay = 0;
  static int temp_limit;
  int c;

  char *endptr;

  while ((c = getopt(argc, argv, "n:w:")) != -1) {
    switch (c) {
    case 'n':
      temp_limit = strtol(optarg, &endptr, 10);
      if (endptr == optarg || *endptr != '\0' || temp_limit < 0) {
        usage_exit();
      }
      limit = &temp_limit;
      break;
    case 'w':
      delay = strtol(optarg, &endptr, 10);
      if (endptr == optarg || *endptr != '\0' || delay < 0) {
        usage_exit();
      }
      break;
    case '?':
      usage_exit();
    }
  }

  int fd;

  // create shared memory
  if ((fd = shm_open(SHM_NAME, SHM_FLAGS, PERM)) == -1) {
    perror("shm_open");
    return EXIT_FAILURE;
  }

  // set size of shared memory
  if (ftruncate(fd, sizeof(shm_t)) == -1) {
    perror("ftruncate");
    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // map shared memory
  shm_t *shm;
  shm = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (shm == MAP_FAILED) {
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

  // initialize shared memory
  shm->terminate = false;

  // initialize circular buffer
  used_slots = sem_used;
  free_slots = sem_free;
  mutex = sem_mutex;
  shm->buffer.read_pos = 0;
  shm->buffer.write_pos = 0;

  // delay
  sleep(delay);

  // smallest number of edges
  int best_solution = -1;
  int num_solution = 0;

  while (!shm->terminate) {

    if (limit && num_solution == *limit || quit) {
      shm->terminate = true;
      printf("The graph might not be 3 colorable, best solution removes %d "
             "edges.\n",
             best_solution);
      break;
    }

    solution_t val;
    do {
      errno = 0;
      val = circ_buf_read(&shm->buffer);
    } while (errno == EINTR);

    if (circ_buf_error != CIRC_BUF_SUCCESS && errno) {
      perror("circ_buf_read");
      sem_unlink(SEM_FREE);

      sem_close(sem_used);
      sem_unlink(SEM_USED);

      sem_close(sem_mutex);
      sem_unlink(SEM_MUTEX);

      shm_unlink(SHM_NAME);
      close(fd);
      return EXIT_FAILURE;
    }
    num_solution++;

    if (val.num_deges == 0) {
      shm->terminate = true;
      printf("The graph is 3-colorable!\n");
      break;
    }

    if (best_solution == -1 || val.num_deges < best_solution) {
      best_solution = val.num_deges;
      fprintf(stderr, "new solution: ");
      for (int i = 0; i < val.num_deges; i++) {
        fprintf(stderr, "%d-%d ", val.edges[i].vertex1_index,
                val.edges[i].vertex2_index);
      }
      fprintf(stderr, "\n");
    }
  }

  // clean up

  // close sem_free
  if (sem_close(sem_free) == -1) {
    perror("sem_close sem_free");
    sem_unlink(SEM_FREE);

    sem_close(sem_used);
    sem_unlink(SEM_USED);

    sem_close(sem_mutex);
    sem_unlink(SEM_MUTEX);

    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // unlink sem_free
  if (sem_unlink(SEM_FREE) == -1) {
    perror("sem_unlink sem_free");
    sem_close(sem_used);
    sem_unlink(SEM_USED);

    sem_close(sem_mutex);
    sem_unlink(SEM_MUTEX);

    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // close sem_used
  if (sem_close(sem_used) == -1) {
    perror("sem_close sem_used");
    sem_unlink(SEM_USED);

    sem_close(sem_mutex);
    sem_unlink(SEM_MUTEX);

    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // unlink sem_used
  if (sem_unlink(SEM_USED) == -1) {
    perror("sem_close sem_used");
    sem_close(sem_mutex);
    sem_unlink(SEM_MUTEX);

    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // close sem_mutex
  if (sem_close(sem_mutex) == -1) {
    perror("sem_close sem_mutex");
    sem_unlink(SEM_MUTEX);

    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // unlink sem_mutex
  if (sem_unlink(SEM_MUTEX) == -1) {
    perror("sem_close sem_used");
    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // unmap shm
  if (munmap(shm, sizeof(shm_t)) == -1) {
    perror("munmap");
    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // unlink shm
  if (shm_unlink(SHM_NAME) == -1) {
    perror("shm_unlink");
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
