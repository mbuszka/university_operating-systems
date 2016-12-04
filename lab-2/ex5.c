/* Maciej Buszka */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "list.h"

#define TRUE 1

typedef struct {
  pthread_t id;
  int       nr;
} thread_info_t;

list_t global_list;

void *reader(void *arg) {
  thread_info_t *info = arg;
  unsigned int rand_state = info->nr;
  char buf[100];
  int found;
  while (TRUE) {
    usleep(rand_r(&rand_state) % 500);
    // doesn't matter if len isn't true length
    if(list_search(&global_list, rand_r(&rand_state) % (global_list.len == 0 ? 1 :global_list.len), &found) == 0) {
      sprintf(buf, "[ READER : %d ] found this number %d\n", info->nr, found);
      // fputs(buf, stdout);
      // fflush_unlocked(stdout);
    } else {
      sprintf(buf, "[ READER : %d ] item not found\n", info->nr);
      // fputs(buf, stdout);
      // fflush_unlocked(stdout);
    }
  }
}

void *writer(void *arg) {
  thread_info_t *info = arg;
  unsigned int rand_state = info->nr;
  char buf[100];
  int item;
  while (TRUE) {
    usleep(rand_r(&rand_state) % 1000);
    if (rand_r(&rand_state) % 2 == 0) {
      item = rand_r(&rand_state);
      list_append(&global_list, item);
      sprintf(buf, "[ WRITER : %d ] appended this number %d\n", info->nr, item);
      // fputs(buf, stdout);
      // fflush_unlocked(stdout);
    } else {
      //doesn't matter if len isn't true length
      item = rand_r(&rand_state) % (global_list.len == 0 ? 1 :global_list.len);
      if (list_remove(&global_list, item) == 0) {
        sprintf(buf, "[ WRITER : %d ] removed at %d\n", info->nr, item);
        // fputs(buf, stdout);
        // fflush_unlocked(stdout);
      } else {
        sprintf(buf, "[ WRITER : %d ] couldn't remove at %d\n", info->nr, item);
        // fputs(buf, stdout);
        // fflush_unlocked(stdout);
      }
    }
  }
}

void spawn(int n, thread_info_t info[]) {
  int result;
  pthread_attr_t attr;
  unsigned int rand_state = rand();

  assert(pthread_attr_init(&attr) == 0);

  for (int i = 0; i < n; i++) {
    info[i].nr = i;
    result = pthread_create(&info[i].id
                          , &attr
                          , rand_r(&rand_state) % 2 == 0 ? reader : writer
                          , &info[i]);
    assert(result == 0);
    // printf("thread spawned\n");
  }
}

void join(thread_info_t threads[], int n) {
  void *thread_return;
  int result;
  for (int i=0; i<n; i++) {
    result = pthread_join(threads[i].id, &thread_return);
    assert(result == 0);
  }
}

int main () {
  int n = 10;
  thread_info_t info[n];

  spawn(n, info);
  join(info, n);

  exit(EXIT_SUCCESS);
}
