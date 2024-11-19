#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#include "../include/circular_buffer.h"
#include "../include/utils.h"

#define DISCARD_MANY_EDGES true

void usage_exit(void) {
  printf("generator EDGE1...\n");
  exit(EXIT_FAILURE);
}

// Parse the graph's edges and return the number of vertices.
int parse(int argc, char **argv, edge_t edges[]) {
  if (argc < 2) {
    printf("no edges given\n");
    usage_exit();
  }

  int num_vertices = 0;
  char *endptr;

  for (int i = 1; i < argc; i++) {
    char *edge_str = argv[i];
    char *dash = strchr(edge_str, '-');

    if (dash == NULL) {
      printf("expected: v1-v2\n");
      usage_exit();
    }

    // parse vertices of the edge
    *dash = '\0';
    int vertex1 = strtol(edge_str, &endptr, 10);
    if (endptr == edge_str || *endptr != '\0' || vertex1 < 0) {
      printf("vertex 1 invalid: %d.\n", vertex1);
      usage_exit();
    }

    int vertex2 = strtol(dash + 1, &endptr, 10);
    if (endptr == dash + 1 || *endptr != '\0' || vertex2 < 0 ||
        vertex1 == vertex2) {
      printf("vertex 2 invalid: %d.\n", vertex2);
      usage_exit();
    }

    // add edge to graph
    edges[i - 1].vertex1_index = vertex1;
    edges[i - 1].vertex2_index = vertex2;

    // update vertices
    if (num_vertices < vertex1 + 1) {
      num_vertices = vertex1 + 1;
    }
    if (num_vertices < vertex2 + 1) {
      num_vertices = vertex2 + 1;
    }
  }

  return num_vertices;
}

int main(int argc, char **argv) {
  printf("Hello Generator!\n");

  // set random seed
  srandom(getpid());

  edge_t edges[argc - 1];

  int num_vertices = parse(argc, argv, edges);

  if (num_vertices == 0) {
    usage_exit();
  }

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

  // initialize semaphores
  used_slots = sem_used;
  free_slots = sem_free;
  mutex = sem_mutex;

  // todo: replace with max 8 edges to be removed
  edge_t removed[argc - 1];
  int num_removed;

  vertex_t vertices[num_vertices];

  // for testing: write value to circular buffer
  circ_buf_write(&shm->buffer, 5);

  if (circ_buf_error != CIRC_BUF_SUCCESS) {
    perror("circ_buf_write");
    sem_unlink(SEM_FREE);

    sem_close(sem_used);
    sem_unlink(SEM_USED);

    sem_close(sem_mutex);
    sem_unlink(SEM_MUTEX);

    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }
  printf("successfully wrote value...\n");
  while (!shm->terminate) {
    num_removed = 0;

    // generate random colours for vertices
    for (int i = 0; i < num_vertices; i++) {
      vertices[i] = get_random_vertex();
      printf("i: %d, c: %d\n", i, vertices[i]);
    }

    // loop through each edge
    for (int i = 0; i < argc - 1; i++) {
      edge_t current = edges[i];

      // check if both vertices are the same
      if (vertices[current.vertex1_index] == vertices[current.vertex2_index]) {
        removed[i] = current;
        num_removed++;
      }
    }

    // write solution to circular buffer
    if (!DISCARD_MANY_EDGES || num_removed <= 8) {
      printf("current solution: ");

      for (int i = 0; i < num_removed; i++) {
        printf("%d-%d,", removed[i].vertex1_index, removed[i].vertex2_index);
      }
      printf("\n");
      printf("\n");
    }
  }

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
  if (munmap(shm, sizeof(shm_t)) == -1) {
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
