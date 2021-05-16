#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "debug.h"

#define DEBUG

int main(int argc, char *argv[]) {
  char *exec_argv[] = { "strace", "ls", NULL, };
  char *exec_envp[] = { "PATH=/bin", NULL, };
  execve("strace",          exec_argv, exec_envp);
  execve("/bin/strace",     exec_argv, exec_envp);
  execve("/usr/bin/strace", exec_argv, exec_envp);
  perror(argv[1]);
  exit(EXIT_FAILURE);
  
}
