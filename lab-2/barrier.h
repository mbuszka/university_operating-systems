/* Maciej Buszka */

#include <semaphore.h>


typedef struct {
  int size;
  // sem_t mutex;
  sem_t in_door;
  sem_t out_door;
} barrier_t;

void bar_wait(barrier_t *b);
void bar_init(barrier_t *b, int n);
void bar_destroy(barrier_t *b);

/*  Koncepcja

  -------------------
  |                 |
          n - 1
  |                 |
  -------------------

  Bariera jest jak śluza, ma dwoje drzwi, zawsze tylko jedne z
  nich jest otwarte.

  Zaczynamy z prawymi zamkniętymi

  1. W poczekalni wchodzi do n - 1 wątków
  2. n-ty wchodząc zamyka lewe drzwi
  3. Otwierane są prawe drzwi i wychodzi n - 1 wątków
  4. Ostatni wychodząc zamyka prawe dzwi i otwiera lewe



*/
