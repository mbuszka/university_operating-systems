#include "strfun.h"
#include <stdbool.h>
#include <string.h>

int strdrop(char *str, const char *set) {
  
  bool map[256] = {false};
  for (const char *c  = set; *c != '\0'; c++)
    map[(int) *c] = true;
  
  const char *r = str;
  char       *w = str;
  do {
    if (!map[(int) *r])
      *w++ = *r;
  } while (*r++ != '\0');
  return w - str - 1;
}

int strcnt(const char *str, const char *set) {
  int cnt = 0;
  
  bool map[256] = {false};
  for (const char *c = set; *c != '\0'; c++)
    map[(int) *c] = true;

  for (const char *r = str; *r != '\0'; r++)
    if (map[(int) *r])
      cnt ++;
  return cnt;
}

