/* Maciej Buszka */

#include "list.h"
#include <stdlib.h>
#include <assert.h>

void list_init(list_t *list) {
  list->head = NULL;
  list->last = NULL;
  list->len  = 0;

  pthread_mutexattr_t mutexattr;
  pthread_mutexattr_init(&mutexattr);
  pthread_condattr_t condattr;
  pthread_condattr_init(&condattr);
  list->searchers = 0;
  pthread_mutex_init(&list->search_mutex, &mutexattr);
  pthread_mutex_init(&list->append_mutex, &mutexattr);
  pthread_mutex_init(&list->remove_mutex, &mutexattr);
  pthread_cond_init(&list->can_remove, &condattr);
}

int list_search(list_t *list, size_t n, int *res) {
  pthread_mutex_lock(&list->search_mutex);
  list->searchers ++;
  pthread_mutex_unlock(&list->search_mutex);

  int status = 1;
  node_t *el = list->head;
  while (n>0 && el != NULL) {
    el = el->next;
    n--;
  }
  if (el != NULL) {
    *res = el->data;
    status = 0;
  }

  pthread_mutex_lock(&list->search_mutex);
  list->searchers --;
  if (list->searchers == 0)
    pthread_cond_broadcast(&list->can_remove);
  pthread_mutex_unlock(&list->search_mutex);
  return status;
}

void list_append(list_t *list, int data) {
  pthread_mutex_lock(&list->append_mutex);

  node_t *el = malloc(sizeof(node_t));
  el->data = data;
  el->next = NULL;

  if (list->head == NULL) {
    list->head = el;
    list->last = el;
  } else {
    list->last->next = el;
    list->last = el;
  }
  list->len++;

  pthread_mutex_unlock(&list->append_mutex);
}

int list_remove(list_t *list, size_t n) {
  pthread_mutex_lock(&list->remove_mutex);
  pthread_mutex_lock(&list->search_mutex);
  pthread_mutex_lock(&list->append_mutex);

  while (list->searchers > 0)
    pthread_cond_wait(&list->can_remove, &list->search_mutex);

  int ok = 1;
  node_t *pr = NULL;
  node_t *el = list->head;

  while (n>0 && el != NULL) {
    pr = el;
    el = el->next;
    n--;
  }
  assert(pr == NULL || pr->next == el);
  if (n == 0 && el != NULL) {
    if (el == list->head) {
      list->head = el->next;
    } else {
      pr->next = el->next;
    }
    if (el == list->last) {
      list->last = pr;
    }
    free(el);
    list->len --;
    ok = 0;
  }

  pthread_mutex_unlock(&list->append_mutex);
  pthread_mutex_unlock(&list->search_mutex);
  pthread_mutex_unlock(&list->remove_mutex);
  return ok;
}
