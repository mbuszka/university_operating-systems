#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "blkio.h"

#define panic()                                                 \
  __extension__({                                               \
    fprintf(stderr, "%s: %s\n", __func__, strerror(errno));     \
    exit(EXIT_FAILURE);                                         \
  })

int blk_open(const char *path) {
  int fd = open(path, O_RDWR|O_DIRECT);

  if (fd < 0)
    panic();

  return fd;
}

void blk_close(int fd) {
  if (close(fd) < 0)
    panic();
}

void blk_read(int fd, void *buf, size_t blkoff, size_t blkcnt) {
  size_t offset = blkoff * BLKSIZE;
  size_t count = blkcnt * BLKSIZE;

  if (lseek(fd, offset, SEEK_SET) < 0) {
    printf("cant seek file\n");
    panic();
  }

  ssize_t actual = read(fd, buf, count);

  if (actual < 0) {
    printf("couldnt read block\n");
    panic();
  }
  assert((size_t)actual == count);
}

void blk_write(int fd, const void *buf, size_t blkoff, size_t blkcnt) {
  size_t offset = blkoff * BLKSIZE;
  size_t count = blkcnt * BLKSIZE;

  if (lseek(fd, offset, SEEK_SET) < 0)
    panic();

  ssize_t actual = write(fd, buf, count);

  if (actual < 0)
    panic();

  assert((size_t)actual == count);
}
