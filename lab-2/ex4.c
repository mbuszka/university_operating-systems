/* Maciej Buszka */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <assert.h>
#include <sys/wait.h>

#define TRUE 1
#define MUTEX_SEM "/mutex_sem"
#define COOK_SEM "/cook_sem"
#define CAULDRON_SEM "/cauldron_sem"

void savage(int n) {
  int goulash_left;
  int cooking;
  sem_t *mutex = sem_open(MUTEX_SEM, O_RDWR);
  assert(mutex != SEM_FAILED);
  sem_t *wakecook = sem_open(COOK_SEM, O_RDWR);
  assert(wakecook != SEM_FAILED);
  sem_t *cauldron = sem_open(CAULDRON_SEM, O_RDWR);
  assert(cauldron != SEM_FAILED);
  while(TRUE) {
    fputs_unlocked("i'm hungry\n", stdout);
    sem_getvalue(cauldron, &goulash_left);
    if (goulash_left == 0) {
      fputs_unlocked("about to wake cook\n", stdout);
      sem_wait(mutex);
      fputs_unlocked("got past mutex\n", stdout);
      sem_getvalue(wakecook, &cooking);
      if (cooking == 0) {
        fputs_unlocked("waking cook\n", stdout);
        sem_post(wakecook);
        sem_post(wakecook);
      }
      sem_post(mutex);
    }
    sem_wait(cauldron);
    char buf[50];
    sprintf(buf, "[ %d ] Yum, tasty goulash!\n", n);
    fputs_unlocked(buf, stdout);
    fflush_unlocked(stdout);
    usleep(rand() % 10000);
  }
}

void cook(int goulash_capacity) {
  int goulash_left;
  sem_t *mutex = sem_open(MUTEX_SEM, O_RDWR);
  sem_t *wakecook = sem_open(COOK_SEM, O_RDWR);
  sem_t *cauldron = sem_open(CAULDRON_SEM, O_RDWR);
  while (TRUE) {
    sem_wait(wakecook);
    fputs_unlocked("cook woke up\n", stdout);
    sem_wait(mutex);
    sem_getvalue(cauldron, &goulash_left);
    assert(goulash_left == 0);
    for (int i=0; i<goulash_capacity; i++) {
      sem_post(cauldron);
    }
    sem_wait(wakecook);
    sem_post(mutex);
  }
}

void cleanup() {
  sem_unlink(MUTEX_SEM);
  sem_unlink(COOK_SEM);
  sem_unlink(CAULDRON_SEM);
}

int main() {
  int res;
  int savages_count = 10;
  int goulash_capacity = 6;
  cleanup();
  assert(sem_open(MUTEX_SEM, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1) != SEM_FAILED);
  assert(sem_open(COOK_SEM, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0) != SEM_FAILED);
  assert(sem_open(CAULDRON_SEM, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, goulash_capacity) != SEM_FAILED);

  if ((res = fork()) < 0) {
    printf("error occured, cannot fork cook\n");
    exit(EXIT_FAILURE);
  } else if (res == 0) {
    cook(goulash_capacity);
  }

  for (int i=0; i<savages_count; i++) {
    if ((res = fork()) < 0) {
      printf("error occured, cannot fork savage %d\n", i);
      exit(EXIT_FAILURE);
    } else if (res == 0) {
      savage(i);
    }
  }

  for (int i=0; i<savages_count; i++) {
    wait(&res);
    assert(res == 0);
  }
  cleanup();
  exit (EXIT_SUCCESS);
}
