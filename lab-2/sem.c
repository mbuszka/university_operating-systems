/* Maciej Buszka */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sem.h"

void sem_init(sem_t *sem, unsigned value) {
  pthread_mutexattr_t lockattr;

  pthread_mutexattr_init(&lockattr);
  pthread_mutexattr_settype(&lockattr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&sem->lock, &lockattr);

  sem->wait = PTHREAD_COND_INITIALIZER;

  sem->count = value;
}

void sem_wait(sem_t *sem) {
  assert(pthread_mutex_lock(&sem->lock) == 0);
  while (sem->count == 0) {
    pthread_cond_wait(&sem->wait, &sem->lock);
  }
  sem->count --;
  assert(pthread_mutex_unlock(&sem->lock) == 0);
}

void sem_post(sem_t *sem) {
  assert(pthread_mutex_lock(&sem->lock) == 0);
  if (sem->count == 0) {
    pthread_cond_signal(&sem->wait);
  }
  sem->count ++;
  assert(pthread_mutex_unlock(&sem->lock) == 0);
}

void sem_getvalue(sem_t *sem, int *sval) {
  assert(pthread_mutex_lock(&sem->lock) == 0);
  *sval = sem->count;
  assert(pthread_mutex_unlock(&sem->lock) == 0);
}
