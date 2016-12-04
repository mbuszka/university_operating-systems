#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include "utils.h"

#define PAGESIZE 4096
#define TRACE_SIZE 20
#define BUFFER_SIZE 1000

void handler(int sig, siginfo_t *info, void *ptr) {
  int err = errno;
  if (sig != SIGSEGV)
    exit(EXIT_FAILURE);
  ucontext_t *context = (ucontext_t *) ptr;
  unsigned char *pc = (unsigned char *) context->uc_mcontext.gregs[REG_RIP];
  unsigned char *sp = (unsigned char *) context->uc_mcontext.gregs[REG_RSP];

  // safely print error message to sderr
  char buf[BUFFER_SIZE];
  int n;
  n = snprintf(buf, BUFFER_SIZE, "SIGSEGV : %s\n"
      " memory address      %lx\n"
      " instruction address %lx\n"
      " stack pointer       %lx\n"
      "Stack trace follows:\n",
          info->si_code == SEGV_ACCERR ? "Invalid permissions" 
                                       : "Address not mapped",
          (unsigned long) info->si_addr,
          (unsigned long) pc,
          (unsigned long) sp) + 1;
  write(2, buf, n);
  errno = err;
  void *trace[TRACE_SIZE];
  backtrace(trace, TRACE_SIZE);
  backtrace_symbols_fd(trace, TRACE_SIZE, 2);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  void *p; char a;
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    handle_error("sigaction");

   /* Allocate a buffer aligned on a page boundary;
      initial protection is PROT_READ | PROT_WRITE */
  
  if (argc >= 2 && 0 == strcmp(argv[1], "readonly")) {
    if (posix_memalign(&p, PAGESIZE, PAGESIZE) < 0)
      handle_error("posix_memalign");
  
    printf("Start of region:        0x%lx\n", (long) p);
    if (mprotect(p, PAGESIZE, PROT_NONE) < 0)
      handle_error("mprotect");

    a =  *((char *) p); //trying to read the memory
    a ++;
  } else if (argc >= 2 && 0 == strcmp(argv[1], "unmapped")) {
    p = NULL;
    a = *((char *) p);
    a ++;
  } else {
    printf("Program usage :\n%s [readonly|unmapped]\n", argv[0]);
    exit(EXIT_SUCCESS);
  }
}
