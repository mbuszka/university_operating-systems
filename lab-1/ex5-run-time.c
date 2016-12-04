#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int (*strdrop)(char *str, const char *set);
int (*strcnt)(const char *str, const char *set);

int main() {
  
  getchar(); 
  void *lib_handle;
  lib_handle = dlopen("./libstrfun.so", RTLD_LAZY);
  
  if (!lib_handle) {
    printf("%s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  dlerror(); // clear any exising errors
  strdrop = (int (*)(char *, const char *)) dlsym(lib_handle, "strdrop");
  
  char *error = dlerror();
  if (error != NULL) {
    printf("%s\n", error);
    exit(EXIT_FAILURE);
  }

  strcnt  = (int (*)(const char *, const char *)) dlsym(lib_handle, "strcnt");
  
  error = dlerror();
  if (error != NULL) {
    printf("%s\n", error);
    exit(EXIT_FAILURE);
  }


  char str[20] = "some fun string";
  char set[10] = "ste";
  char look[10] = "urg";
  printf("we dropped \"%s\" from \"%s\" ", set, str);
  int len = (*strdrop)(str, set);
  printf("and got \"%s\" of length %d\n", str, len);
  printf("It has %d characters from \"%s\" set\n",
          (*strcnt)(str, look),
          look);
  getchar();
  dlclose(lib_handle);
  getchar();
  return 0;
}
