/* Maciej Buszka */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <signal.h>
#include <wait.h>

#define TRUE 1
#define SEM_WAITER "/semwaiter"
#define SEM_FORK_PREFIX "/semfork"

typedef struct {
  int    nr;
  sem_t *left;
  sem_t *right;
  sem_t *waiter;
} philosopher_info_t;

int    philosopher_count = 10000;
pid_t *philosophers;

void philosopher(int i);

void initialize_semaphores() {
  char buf[50];
  sem_t *sem;
  for (int i=0; i<philosopher_count; i++) {
    sprintf(buf, "%s%d", SEM_FORK_PREFIX, i);
    sem = sem_open( buf
                  , O_CREAT | O_EXCL
                  , S_IRUSR | S_IWUSR
                  , 1);
    assert(sem != SEM_FAILED);
  }
  sem = sem_open( SEM_WAITER
                , O_CREAT | O_EXCL
                , S_IRUSR | S_IWUSR
                , philosopher_count - 1);
  assert(sem != SEM_FAILED);
}

void unlink_semaphores() {
  char buf[50];
  for (int i=0; i<philosopher_count; i++) {
    sprintf(buf, "%s%d", SEM_FORK_PREFIX, i);
    sem_unlink(buf);
  }
  sem_unlink(SEM_WAITER);
}

void spawn() {
  int result;
  philosophers = calloc(philosopher_count, sizeof(pid_t));
  for (int i=0; i<philosopher_count; i++) {
    result = fork();
    assert(result >= 0);
    if (result == 0) {
      philosopher(i);
    } else {
      philosophers[i] = result;
    }
  }
}

void waitchld() {
  int status;
  for (int i=0; i<philosopher_count; i++) {
    wait(&status);
  }
}

void handler(int signum) {
  assert(signum == SIGINT);
  for (int i=0; i<philosopher_count; i++) {
    kill(philosophers[i], SIGKILL);
  }
}

void setup() {
  struct sigaction action;

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  assert(sigaction(SIGINT, &action, NULL) == 0);
}

int main() {
  unlink_semaphores();
  initialize_semaphores();
  spawn();
  setup();
  waitchld();
  unlink_semaphores();
  return 0;
}

void open_semaphores(philosopher_info_t *info) {
  char buf[50];
  sprintf(buf, "%s%d", SEM_FORK_PREFIX, info->nr);
  info->left = sem_open(buf, O_RDWR);
  assert(info->left != SEM_FAILED);

  sprintf(buf, "%s%d", SEM_FORK_PREFIX, (info->nr+1)%philosopher_count);
  info->right = sem_open(buf, O_RDWR);
  assert(info->right != SEM_FAILED);

  info->waiter = sem_open(SEM_WAITER, O_RDWR);
  assert(info->waiter != SEM_FAILED);
}

void think(philosopher_info_t *info) {
  char buf[50];
  sprintf(buf, "[ %d ] thinking\n", info->nr);
  fputs_unlocked(buf, stdout);
  fflush_unlocked(stdout);
  usleep(rand() % 10000);
}

void eat(philosopher_info_t *info) {
  char buf[50];
  sprintf(buf, "[ %d ] eating\n", info->nr);
  fputs_unlocked(buf, stdout);
  fflush_unlocked(stdout);
  usleep(rand() % 10000);
}

void take_forks(philosopher_info_t *info) {
  sem_wait(info->waiter);
  sem_wait(info->left);
  sem_wait(info->right);
}

void put_forks(philosopher_info_t *info) {
  sem_post(info->right);
  sem_post(info->left);
  sem_post(info->waiter);
}

void philosopher(int i) {
  philosopher_info_t info;
  info.nr = i;
  open_semaphores(&info);

  while (TRUE) {
    think(&info);
    take_forks(&info);
    eat(&info);
    put_forks(&info);
  }
}
