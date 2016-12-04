#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include "utils.h"

void daemonize(char *cmd) {
  int fd0,          fd1, fd2;
  pid_t             pid;
  struct rlimit     rl;
  struct sigaction  sa;

  // clear file creation mask
  umask(0); 
  
  // get maximum number of file descriptors
  if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    handle_error("can't get the file limit");
  
  // become a session leader, so we don't control the terminal
  if ((pid = fork()) < 0) {
    handle_error("can't fork");
  } else if (pid != 0) {
    exit(EXIT_SUCCESS);
  }
  setsid();
  
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGHUP, &sa, NULL) < 0)
    handle_error("can't ignore the SIGHUP");
  if ((pid = fork()) < 0)
    handle_error("can't fork");
  else if (pid != 0)
    exit(EXIT_SUCCESS);
  
  if (chdir("/") < 0)
    handle_error("can't change working dir to /");

  if (rl.rlim_max == RLIM_INFINITY)
    rl.rlim_max = 1024;
  for (unsigned int i = 0; i < rl.rlim_max; i ++)
    close(i);

  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  fd2 = dup(0);

  openlog(cmd, LOG_CONS, LOG_DAEMON);
  if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
    syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
    exit(EXIT_FAILURE);
  }
  syslog(LOG_INFO, "the daemon is initiated");
}

int main(int argc, char *argv[]) {
  if (argc < 1) exit(EXIT_FAILURE);
  char     *myname = basename(argv[0]);
  int       sig;
  siginfo_t si;
  sigset_t  signals;
  int       cnt;
  
  daemonize(myname);
  
  sigfillset(&signals);
  if (sigprocmask(SIG_SETMASK, &signals, NULL) < 0) {
    syslog(LOG_ERR, "cannot block signals");
    exit(EXIT_FAILURE);
  }

  while (TRUE) {
    sig = sigwaitinfo(&signals, &si);
    if (sig < 0) {
      syslog(LOG_ERR, "signal wait failed");
      exit(EXIT_FAILURE);
    }

    if (sig == SIGTERM) {
      syslog(LOG_INFO, "SIGTERM received, closing daemon, value %d", cnt);
      break;
    } else if (sig == SIGUSR1) {
      cnt ++;
      syslog(LOG_INFO, "counter has been incremented, value %d", cnt);
    } else if (sig == SIGHUP) {
      cnt = 0;
      syslog(LOG_INFO, "counter has been reset");
    }
  }

  return 0;
}

