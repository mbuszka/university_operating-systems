/* Maciej Buszka */

#include "barrier.h"

void bar_init(barrier_t *b, int n) {
  // sem_init(b->mutex, 1);
  b->size = n;
  sem_init(&b->in_door, 1, n);
  sem_init(&b->out_door, 1, 0);
}

void bar_wait(barrier_t *b) {
  sem_wait(&b->in_door);
  int free_room;
  int n = b->size;
  sem_getvalue(&b->in_door, &free_room);
  if (free_room == 0) {
    for (int i=0; i < n-1; i++) {
      sem_post(&b->out_door);
    }
    for (int i=0; i<n; i++) {
      sem_post(&b->in_door);
    }
  } else {
    sem_wait(&b->out_door);
  }
}

void bar_destroy(barrier_t *b) {
  sem_destroy(&b->in_door);
  sem_destroy(&b->out_door);
}
