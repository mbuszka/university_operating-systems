/* Maciej Buszka */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "sem.h"

#define TRUE  1
#define FALSE 0

sem_t smoke;
sem_t wake[3];
sem_t resources[3];
sem_t proxy_sync;
int   available[3] = { 0, 0, 0 };
int   types[3] = { 0, 1, 2 };

void agent();
void *proxy(void *arg);
void *smoker(void *arg);

int main () {
  int result;
  pthread_attr_t attr;
  pthread_t child;

  assert(pthread_attr_init(&attr) == 0);

  sem_init(&smoke, 1);
  sem_init(&proxy_sync, 1);
  for (int i=0; i<3; i++) {
    sem_init(&wake[i], 0);
    sem_init(&resources[i], 0);
  }


  for (int i=0; i<3; i++) {
    result = pthread_create(&child
                          , &attr
                          , smoker
                          , &types[i]);
    assert(result == 0);

    result = pthread_create(&child
                          , &attr
                          , proxy
                          , &types[i]);
    assert(result == 0);
  }
  printf("Starting agent\n");
  agent();
  return 0;
}

void agent() {
  int x;
  // char buf[50];
  while (TRUE) {
    sem_wait(&smoke);
    x = rand() % 3;
    // sprintf(buf, "Agent woke up with %d %d\n", x, (x+1)%3);
    // fputs_unlocked(buf, stdout);
    // fflush_unlocked(stdout);
    sem_post(&resources[x]);
    sem_post(&resources[(x+1)%3]);
  }
}

void *proxy(void *arg) {
  int type = *((int*) arg);
  // char buf[50];
  while (TRUE) {
    sem_wait(&resources[type]);
    sem_wait(&proxy_sync);
    // sprintf(buf, "Proxy %d woke up\n", type);
    // fputs_unlocked(buf, stdout);
    if (available[(type+1)%3]) {
      available[(type+1)%3] = FALSE;
      sem_post(&resources[(type+2)%3]);
    } else if (available[(type+2)%3]) {
      available[(type+2)%3] = FALSE;
      sem_post(&resources[(type+1)%3]);
    } else {
      available[type] = TRUE;
    }
    sem_post(&proxy_sync);
  }
}

void *smoker(void *arg) {
  int type = *((int*) arg);
  char buf[50];
  while (TRUE) {
    sem_wait(&resources[type]);
    sprintf(buf, "Smoking %d\n", type);
    fputs_unlocked(buf, stdout);
    // fflush_unlocked(stdout);
    sem_post(&smoke);
    usleep(100 + rand()%1000);
  }
}
