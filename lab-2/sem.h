/* Maciej Buszka */

#include <pthread.h>

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t  wait;
  unsigned count;
} sem_t;

void sem_init(sem_t *sem, unsigned value);
void sem_wait(sem_t *sem);
void sem_post(sem_t *sem);
void sem_getvalue(sem_t *sem, int *sval);
