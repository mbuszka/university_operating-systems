/* Maciej Buszka */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>
#include "barrier.h"
#include <errno.h>

#define SHARED_NAME "/horse_gates"

barrier_t *barrier;
int barrier_fd;
int rounds;

void horse(int n);

barrier_t *initialize_barrier(int n) {
  barrier_t *barrier;
  barrier_fd = shm_open(SHARED_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  assert(barrier_fd != 0);
  ftruncate(barrier_fd, sizeof(barrier_t));
  barrier = mmap( NULL, sizeof(barrier_t)
                , PROT_READ | PROT_WRITE
                , MAP_SHARED
                , barrier_fd
                , 0
                );
  assert(barrier != NULL);
  bar_init(barrier, n);
  return barrier;
}

void destroy_barrier() {
  bar_destroy(barrier);
  munmap(barrier, sizeof(barrier_t));
  shm_unlink(SHARED_NAME);
}

void spawn_horses(int horse_count, pid_t horses[horse_count]) {
  int result;
  for (int i=0; i<horse_count; i++) {
    result = fork();
    if (result < 0) {
      printf("error, cannot fork, %d", errno);
    }
    if (result == 0) {
      horse(i);
      exit(EXIT_SUCCESS);
    } else {
      horses[i] = result;
    }
  }
}

int main() {
  int n = 10;
  int horse_count = 2 * n;
  rounds = 5;
  pid_t horses[horse_count];

  barrier = initialize_barrier(n);

  spawn_horses(horse_count, horses);
  int status;
  for (int i=0; i<horse_count; i++) {
    wait(&status);
  }

  destroy_barrier();
  exit(EXIT_SUCCESS);
}

void horse(int n) {
  for (int i=0; i<rounds; i++) {
    bar_wait(barrier);
    char buf[50];
    sprintf(buf, "[ %d ] started running, lap %d\n", n, i);
    fputs_unlocked(buf, stdout);
    fflush_unlocked(stdout);
    usleep(rand() % 10000);
  }
}
