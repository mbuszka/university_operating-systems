#include <unistd.h>
#include <ucontext.h>
#include <ctype.h>
#include "utils.h"

#define BUF_SIZE 1000

char inbuf[BUF_SIZE];
char outbuf[BUF_SIZE];
volatile int  finished = FALSE;

static ucontext_t context[4];

static void reader() {
  int wrdcnt = 0;
  while (scanf("%s", inbuf) != EOF) {
    wrdcnt ++;
    if (swapcontext(context, context + 1) == -1 ) {
      handle_error("swapcontext");
    }
  }
  finished = TRUE;
  printf("Counters:\n words : %d\n", wrdcnt);
  swapcontext(context, context + 2);
  return;
}

static void remover() {
  while (TRUE) {
    char *inptr = inbuf;
    char *outptr = outbuf;
    do {
      if (isalnum(*inptr)) {
        *(outptr++) = *inptr;
      }
    } while (*(inptr++) != '\0');
    *outptr = '\0';
    if (swapcontext(&context[1], &context[2]) == -1 ) {
      handle_error("swapcontext");
    }
  }
  printf("hello!\n");
}

static void writer() {
  int ltrcnt = 0;
  
  while (!finished) {
    printf("%s\n", outbuf);
    ltrcnt += strlen(outbuf);
    if (swapcontext(&context[2], &context[0]) == -1 ) {
      handle_error("swapcontext");
    }
  }
  printf(" characters : %d\n", ltrcnt);
}



int main() {
  char stack[3][10000];

  for (int i = 0; i < 3; i ++) {
    if (getcontext(&context[i]) == -1) {
      handle_error("getcontext");
    }
    context[i].uc_stack.ss_sp = stack[i];
    context[i].uc_stack.ss_size = sizeof(stack[i]);
    context[i].uc_link = &context[3];
  }
  makecontext(&context[0], reader, 0);
  makecontext(&context[1], remover, 0);
  makecontext(&context[2], writer, 0);
  
  if (swapcontext(&context[3], &context[0]) == -1) {
    handle_error("swapcontext");
  }

  exit(EXIT_SUCCESS);
}
