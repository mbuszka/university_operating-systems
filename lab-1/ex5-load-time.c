#include <stdio.h>
#include "strfun.h"

int main() {
  char str[20] = "some fun string";
  char set[10] = "ste";
  char look[10] = "urg";
  printf("we dropped \"%s\" from \"%s\" ", set, str);
  int len = strdrop(str, set);
  printf("and got \"%s\" of length %d\n", str, len);
  printf("It has %d characters from \"%s\" set\n",
          strcnt(str, look),
          look);
  return 0;
}
