#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

#define TRUE 1
#define FALSE 0

#define handle_error(e_msg) \
  do {                                                                \
    char err_str[50]; \
    strerror_r(errno, err_str, 50); \
    printf("An error occured, msg: %s, error: %s\n", e_msg, err_str);\
    exit(EXIT_FAILURE); \
  } while (0)
#endif
