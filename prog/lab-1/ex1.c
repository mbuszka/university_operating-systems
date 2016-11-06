#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include "utils.h"

#define ARGS_SIZE 10


void child_work();
void parent_work();

extern char **environ;

void spawn_zombie() {
  switch (fork()) {
    case -1 : handle_error("fork");
    case  0 : printf("[ %d ] I'm a zombie\n", getpid());
              _exit(EXIT_SUCCESS);
              break;
    default : return;
  }
}

void fork_ps(char *progname) {
  char *args[ARGS_SIZE] = { "ps", "-C", progname,"-o", "pid,ppid,s,comm",  NULL };
  switch (fork()) {
    case -1 : handle_error("fork");
    case  0 : if (execve("/usr/bin/ps", args, environ) == -1) {
                handle_error("execve");
              }
              break;
    default : return;
  }
}

void ignore_sigchld() {
  struct sigaction sa;
  
  sa.sa_handler = SIG_IGN;
  sa.sa_flags   = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
    handle_error("sig_action");
  return;
}

int main(int argc, char *argv[]) {
  pid_t mypid  = getpid();
  char *myname = basename(argv[0]);
  
  if (argc >= 2 && strcmp(argv[1], "-i") == 0) {
    ignore_sigchld();
  } else if (argc >= 2) {
    printf("Usage: %s [-i]\n\t-i -  ignore SIG_CHLD\n", myname);
    exit(EXIT_SUCCESS);
  }

  spawn_zombie();
  printf("[ %d ] Press any key to continue\n", mypid);
  getchar();
  fork_ps(myname);
  sleep(1);

  return EXIT_SUCCESS;
}
