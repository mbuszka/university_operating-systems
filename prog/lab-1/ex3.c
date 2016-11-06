#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include "utils.h"

#define BUF_SIZE 1000

typedef struct pipe {
  int source;
  int sink;
} Pipe;

Pipe read_rem;
Pipe rem_write;

void init_pipe(Pipe *p) {
  int fd[2];
  if (pipe(fd) == -1)
    handle_error("init_pipe");
  p->source = fd[0];
  p->sink   = fd[1];
}

void reader() {
  int bytes_read;
  char inbuf[BUF_SIZE];
  int wrdcnt = 0;
  while (scanf("%s", inbuf) != EOF ) {
    wrdcnt ++;
    bytes_read = strlen(inbuf) + 1;
    // printf("Reader : sent %d bytes, str : %s\n", bytes_read, inbuf);
    write(read_rem.sink, inbuf, bytes_read);
  }
  printf("Counters:\n words : %d\n", wrdcnt);
  close(read_rem.sink);
}

void remover() {
  char buf[BUF_SIZE];
  int bytes_read;
  
  while ((bytes_read = read(read_rem.source, buf, BUF_SIZE)) != 0) {
    // printf("Remover : received %d bytes, str : %s\n", bytes_read, buf);
    char *inptr = buf;
    char *outptr = buf;
    while (inptr != buf + bytes_read) {
      do {
        if (isalnum(*inptr)) {
          *(outptr++) = *inptr;
        }
      } while (*(inptr++) != '\0');
      *outptr++ = '\0';
    }
    // printf("Remover : sent %ld bytes, str : %s\n", outptr-buf , buf);
    write(rem_write.sink, buf, outptr-buf);
  }
  close(read_rem.source);
  close(rem_write.sink);
}

void writer() {
  char buf[BUF_SIZE];
  int ltrcnt = 0;
  int bytes_read;
  while (bytes_read = read(rem_write.source, buf, BUF_SIZE)) {
    char *ptr = buf;
    while (ptr != buf + bytes_read) {
      printf("%s\n", ptr);
      int n = strlen(ptr);
      ptr += n + 1;
      ltrcnt += n;
    }
  }
  printf(" letters : %d\n", ltrcnt);
  close(rem_write.source);
}

pid_t spawn_reader() {
  pid_t s;
  switch (s = fork()) {
    case -1 : handle_error("spawn_reader");
    case  0 : close(read_rem.source);
              reader();
              exit(EXIT_SUCCESS);
    default : return s;
  }
}

pid_t spawn_remover() {
  pid_t s;
  switch (s = fork()) {
    case -1 : handle_error("spawn_remover");
    case  0 : close(read_rem.sink);
              close(rem_write.source);
              remover();
              exit(EXIT_SUCCESS);
    default : return s;
  }
}

pid_t spawn_writer() {
  pid_t s;
  switch(s = fork()) {
    case -1 : handle_error("spawn_writer");
    case  0 : close(rem_write.sink);
              writer();
              exit(EXIT_SUCCESS);
    default : return s;
  }
}

int main() { /*
  char buf[BUF_SIZE];
  int n = scanf("%s", buf);
  printf("%d", n ++); */
  pid_t reader_pid, remover_pid, writer_pid;
  int status;

  init_pipe(&read_rem);

  reader_pid = spawn_reader();
  
  init_pipe(&rem_write);

  remover_pid = spawn_remover();
  
  close(read_rem.source);
  close(read_rem.sink);

  writer_pid = spawn_writer();

  close(rem_write.source);
  close(rem_write.sink);
  
  for (int i=0; i<3; i++) {
    wait(&status);
  }

  exit(EXIT_SUCCESS);
}
