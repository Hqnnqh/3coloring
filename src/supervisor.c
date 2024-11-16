#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHM_NAME "/shared_memory_3coloring"
#define MAX_DATA (50)

struct myshm {
  unsigned int state;
  unsigned int data[MAX_DATA];
};

void usage_exit(void) {
  printf("supervisor [-n limit] [-w delay]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  printf("Hello World\n");

  // delay is infinite if it remains a NULL pointer
  int limit = 0, *delay = NULL;
  static int temp_delay;
  int c;

  char *endptr;

  while ((c = getopt(argc, argv, "n:w:")) != -1) {
    switch (c) {
    case 'n':
      limit = strtol(optarg, &endptr, 10);
      if (endptr == optarg || *endptr != '\0' || limit < 0) {
        usage_exit();
      }
      break;
    case 'w':
      temp_delay = strtol(optarg, &endptr, 10);
      if (endptr == optarg || *endptr != '\0' || temp_delay < 0) {
        usage_exit();
      }
      delay = &temp_delay;
      break;
    case '?':
      usage_exit();
    }
  }

  int fd;

  // create shared memory
  if ((fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600)) == -1) {
    perror("shm_open");
    return EXIT_FAILURE;
  }

  // set size of shared memory
  if (ftruncate(fd, sizeof(struct myshm)) == -1) {
    perror("ftruncate");
    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // map shared memory
  struct myshm *shmptr;
  shmptr = mmap(NULL, sizeof(struct myshm), PROT_READ | PROT_WRITE, MAP_SHARED,
                fd, 0);

  if (shmptr == MAP_FAILED) {
    perror("mmap");
    shm_unlink(SHM_NAME);
    close(fd);
    return EXIT_FAILURE;
  }

  // clean up

  // unmap shm
  if (munmap(shmptr, sizeof(struct myshm)) == -1) {
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
