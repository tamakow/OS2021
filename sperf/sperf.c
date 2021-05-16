#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>

#define DEBUG
#define __USE_GNU
#include <unistd.h>
#include "debug.h"

#define NAME_LEN 64

typedef struct syscall_node {
  char   name[NAME_LEN];
  double time;
  struct syscall_node *next;
} syscall_node_t;

syscall_node_t *head = NULL;


int main(int argc, char *argv[]) {
  if(argc < 2) {
    Assert(FONT_RED, "Invalid arguements!\nUsage: ./sperf-64 [cmd] [args]");
  }

  int fildes[2]; // 0: read 1: write
  char **exec_argv;
  char **exec_envp = environ;
  char *path = getenv("PATH");
  char *exec_path = (char*)malloc(10 + strlen(path));
  

  
  //pipe [0 : stdin] [1 : stdout] [2 : stderr]
  if(pipe(fildes) < 0) {
    Assert(FONT_YELLOW, "Pipe failed");
  }
  Log("%d %d",fildes[0],fildes[1]);

  exec_argv = (char**)malloc(sizeof(char*) * (argc + 3));
  exec_argv[0] = "strace";
  exec_argv[1] = "-T";
  exec_argv[2] = "-xx";
  memcpy(exec_argv + 3, argv + 1, argc * sizeof(char*));
  for(int i = 0; i < argc + 3; ++i)
    Log("%s",exec_argv[i]);
  
  // strcat(exec_path, "PATH=");
  // strcat(exec_path, path);
  // Log("%s", exec_path);


  pid_t pid = fork();
  if(pid < 0) {
    Assert(FONT_BLUE, "Fork failed");
  }

  if(pid == 0) { 
    close(fildes[0]);
    int blackhole = open("/dev/null", O_RDWR | O_APPEND);
    if(blackhole == -1) Assert(FONT_CYAN, "Open /dev/null failed");
    dup2(blackhole, STDOUT_FILENO);
    dup2(fildes[1], STDERR_FILENO); 
    // strace must be in some place in the ath
    strcat(exec_path, "strace");
    char *token = strtok(path, ":"); // path can't be used after the operations
    Log("initial exec_path is %s\ntoken is %s",exec_path,token);
    while(execve(exec_path, exec_argv, exec_envp) == -1) {
      memset(exec_path, 0, strlen(exec_path));
      strcat(exec_path, token);
      strcat(exec_path, "/strace");
      token = strtok(NULL, ":");
      Log("try exec_path: %s",exec_path);
    }
    Assert(FONT_RED, "Should not reach here!");
  } else {

  }
}
