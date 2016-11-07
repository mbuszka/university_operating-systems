#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include "utils.h"

#define ARGS_SIZE 10

extern char **environ;
char *myname;


void spawn_zombie() {
  switch (fork()) {
    case -1 : handle_error("fork");
    case  0 : printf("[ %d ] I'm a zombie\n", getpid());
              exit(EXIT_SUCCESS);
              break;
    default : return;
  }
}

void fork_ps(char *progname) {
  char *args[ARGS_SIZE] = { "ps", "-C", progname, "-o", "pid,ppid,s,comm",  NULL };
  switch (fork()) {
    case -1 : handle_error("fork");
    case  0 : if (execve("/usr/bin/ps", args, environ) == -1) {
                handle_error("execve");
              }
              break;
    default : return;
  }
}

void spawn_orphan() {
  switch (fork()) {
    case -1 : handle_error("fork");
    case  0 : printf("[ %d ] I'm an orphan\n", getpid());
              sleep(1);
              fork_ps(myname);
              sleep(2);
              exit(EXIT_SUCCESS);
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
  myname = basename(argv[0]);
  
  setbuf(stdout, NULL);

  if (argc >= 2 && strcmp(argv[1], "-i") == 0) {
    ignore_sigchld();
  }
  if ((argc == 2 && 0 == strcmp(argv[1], "orphan")) || 
      (argc == 3 && 0 == strcmp(argv[2], "orphan"))) {
    spawn_orphan();
    exit(EXIT_SUCCESS);

  } else if ((argc == 2 && 0 == strcmp(argv[1], "zombie")) ||
             (argc == 3 && 0 == strcmp(argv[2], "zombie"))) {
    printf("[ %d ] Spawning zombie\n", mypid);
    spawn_zombie();
    printf("[ %d ] Press any key to continue\n", mypid);
    getchar();
    fork_ps(myname);
    sleep(1);

  } else {
    printf("Usage: %s [-i] <zombie|orphan>\n\t-i  - ignore SIG_CHLD\n", myname);
    exit(EXIT_SUCCESS);
  }

  return EXIT_SUCCESS;
}
