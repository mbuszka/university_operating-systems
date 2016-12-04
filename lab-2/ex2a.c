/* Maciej Buszka */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "sem.h"

/* ustaw maski sygnałów, żeby main dostał SIGINT
   ustaw thread cancellation type na asynchronous */

#define TRUE 1

typedef struct {
  pthread_t id;
  int       nr;
  sem_t    *left_fork;
  sem_t    *right_fork;
} thread_info_t;

sem_t *forks;
sem_t *waiter;

void *philosopher(void *i);
thread_info_t *spawn(int n);
void join(thread_info_t* threads, int n);

int main(int argc, char *argv[]) {
  int philosopher_count = 10;
  if (argc == 2) {
    philosopher_count = atoi(argv[1]);
  }
  thread_info_t *threads;

  forks = calloc(philosopher_count, sizeof(sem_t));
  for (int i=0; i<philosopher_count; i++) {
    sem_init(&forks[i], 1);
  }

  waiter = malloc(sizeof(sem_t));
  sem_init(waiter, philosopher_count-1);

  threads = spawn(philosopher_count);
  join(threads, philosopher_count);

  return EXIT_SUCCESS;
}


thread_info_t *spawn(int n) {
  int result;
  int *thread_return;
  pthread_attr_t attr;
  thread_info_t *thread_info;

  thread_info = calloc(n, sizeof(thread_info_t));
  assert(thread_info != NULL);

  assert(pthread_attr_init(&attr) == 0);

  for (int i = 0; i < n; i++) {
    thread_info[i].nr = i+1;
    thread_info[i].left_fork = &forks[i];
    thread_info[i].right_fork = &forks[(i+1)%n];
    result = pthread_create(&thread_info[i].id
                          , &attr
                          , philosopher
                          , &thread_info[i]);
    assert(result == 0);
  }

  return thread_info;
}

void join(thread_info_t *threads, int n) {
  thread_info_t *thread_return;
  int result;
  for (int i=0; i<n; i++) {
    result = pthread_join(threads[i].id, &thread_return);
    assert(result == 0);
  }
}


void take_forks(thread_info_t *info) {
  sem_wait(waiter);
  sem_wait(info->left_fork);
  sem_wait(info->right_fork);
}

void put_forks(thread_info_t *info) {
  sem_post(info->right_fork);
  sem_post(info->left_fork);
  sem_post(waiter);
}

void think(thread_info_t *info) {
  char buf[50];
  sprintf(buf, "[ %d ] thinking\n", info->nr);
  fputs_unlocked(buf, stdout);
  fflush_unlocked(stdout);
  usleep(rand() % 10000);
}

void eat(thread_info_t *info) {
  char buf[50];
  sprintf(buf, "[ %d ] eating\n", info->nr);
  fputs_unlocked(buf, stdout);
  fflush_unlocked(stdout);
  usleep(rand() % 10000);

}

void *philosopher(void *arg) {
  thread_info_t *info = arg;
  while (TRUE) {
    think(info);
    take_forks(info);
    eat(info);
    put_forks(info);
  }
  return info;
}
