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

void usage_exit(void) {
  printf("generator EDGE1...\n");
  exit(EXIT_FAILURE);
}

graph_t parse(int argc, char **argv) {
  if (argc < 2) {
    printf("no edges given\n");
    usage_exit();
  }

  graph_t graph;
  graph.edges = NULL;
  graph.vertices = NULL;
  graph.num_edges = 0;
  graph.num_vertices = 0;

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
    graph.num_edges++;
    edge_t *new_edges = realloc(graph.edges, graph.num_edges * sizeof(edge_t));

    if (new_edges == NULL) {
      perror("realloc edges");
      free_graph(&graph);
      exit(EXIT_FAILURE);
    }
    graph.edges = new_edges;
    graph.edges[graph.num_edges - 1].vertex1_index = vertex1;
    graph.edges[graph.num_edges - 1].vertex2_index = vertex2;

    // update vertices
    if (graph.num_vertices < vertex1 + 1) {
      graph.num_vertices = vertex1 + 1;
    }
    if (graph.num_vertices < vertex2 + 1) {
      graph.num_vertices = vertex2 + 1;
    }
  }

  // initialize vertices
  graph.vertices = malloc(graph.num_vertices * sizeof(vertex_t));

  if (graph.vertices == NULL) {
    perror("malloc vertices");
    exit(EXIT_FAILURE);
  }

  return graph;
}

int main(int argc, char **argv) {
  printf("Hello Generator!\n");

  // set random seed
  srandom(getpid());

  graph_t graph = parse(argc, argv);

  // assign random colours to vertices
  for (int i = 0; i < graph.num_vertices; i++) {
    graph.vertices[i] = get_random_vertex();
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
  struct circ_buf *circ_buf_ptr;
  circ_buf_ptr = mmap(NULL, sizeof(struct circ_buf), PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);

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

  // attempt to solve graph




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
