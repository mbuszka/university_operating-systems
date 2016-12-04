/* Maciej Buszka */

#include <pthread.h>

typedef struct _node_t {
  int     data;
  struct _node_t *next;
} node_t;

// Implementuje monitor nadzorujący operacje na liscie
typedef struct {
  node_t *head;
  node_t *last;
  size_t  len;

  size_t searchers;
  pthread_mutex_t search_mutex;
  pthread_mutex_t append_mutex;
  pthread_mutex_t remove_mutex;
  pthread_cond_t  can_remove;
} list_t;

void list_init(list_t *list);
int  list_search(list_t *list, size_t n, int *res);
void list_append(list_t *list, int data);
int  list_remove(list_t *list, size_t n);

// priorytet
//          <         <
//  search    append    remove
//     n        0/1       0
//     0         0        1

// lista jednokierunkowa

/* narzędzia:
    mutex
    condvar
    semafor (zadanie 1)
*/

/* trzy mutexy
  mutex_r - używany przez czytających
  mutex_a - używany przez dodających
  mutex_d -
*/

/* search
    // należy dodać jakiś sposób na niegłodzenie removera
    lock   mutex_r
    wait(no_removers, mutex_r)
    readers ++
    unlock mutex_r
    ...
    lock   mutex_r
    readers --
    if readers == 0
      signal no_readers
    unlock mutex_r

  append
    lock   mutex_a
    ...
    unlock mutex_a

  delete
    lock mutex_d

    lock mutex_r
      while readers > 0
        wait(no_reader, mutex_r)



    broadcast(no_removers)
*/
