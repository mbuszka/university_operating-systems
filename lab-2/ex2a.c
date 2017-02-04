/* Maciej Buszka */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
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
int philosopher_count = 10000;
thread_info_t *threads;

void *philosopher(void *i);
void spawn();
void join();

static void handler(int signum) {
  assert(signum == SIGINT);
  for (int i=0; i<philosopher_count; i++) {
    pthread_cancel(threads[i].id);
  }
}

int main(int argc, char *argv[]) {
  struct sigaction action;

  if (argc == 2) {
    philosopher_count = atoi(argv[1]);
  }

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  assert(sigaction(SIGINT, &action, NULL) == 0);

  forks = calloc(philosopher_count, sizeof(sem_t));
  for (int i=0; i<philosopher_count; i++) {
    sem_init(&forks[i], 1);
  }

  waiter = malloc(sizeof(sem_t));
  sem_init(waiter, philosopher_count-1);

  spawn();
  join();

  free(waiter);
  free(forks);
  return EXIT_SUCCESS;
}

void spawn() {
  int result;
  pthread_attr_t attr;

  threads = calloc(philosopher_count, sizeof(thread_info_t));
  assert(threads != NULL);

  assert(pthread_attr_init(&attr) == 0);

  for (int i = 0; i < philosopher_count; i++) {
    threads[i].nr = i+1;
    threads[i].left_fork = &forks[i];
    threads[i].right_fork = &forks[(i+1)%philosopher_count];
    result = pthread_create(&threads[i].id
                          , &attr
                          , philosopher
                          , &threads[i]);
    assert(result == 0);
  }
}

void join() {
  thread_info_t *thread_return;
  int result;
  for (int i=0; i<philosopher_count; i++) {
    result = pthread_join(threads[i].id, (void *) &thread_return);
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
  sigset_t sigset;
  int oldtype;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  pthread_sigmask(SIG_BLOCK, &sigset, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

  while (TRUE) {
    think(info);
    take_forks(info);
    eat(info);
    put_forks(info);
  }
  return info;
}
