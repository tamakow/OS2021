#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>


#define DEBUG
#include "debug.h"

#define NAME_LEN 64

typedef struct syscall_node {
  char   name[NAME_LEN];
  double time;
  struct syscall_node *next;
} syscall_node_t;

syscall_node_t *head = NULL;
double total_time = 0;

void display() {
  //TODO
  printf("====================\n");
  for(int i = 0; i < 80; ++i)
    putc('\0', stdout);
  fflush(stdout);
}


int main(int argc, char *argv[]) {
  if(argc < 2) {
    Assert(FONT_RED, "Invalid arguements!\nUsage: ./sperf-64 [cmd] [args]");
  }

  int fildes[2]; // 0: read 1: write
  char **exec_argv;
  char **exec_envp = __environ;
  char path[1024];
  char exec_path[1024];
  char file_path[] = "strace_output";

  //pipe [seems no use in my code]
  // if(pipe(fildes) < 0) {
  //   Assert(FONT_YELLOW, "Pipe failed");
  // }
  // Log("%d %d",fildes[0],fildes[1]);

  strcpy(path, getenv("PATH"));
  exec_argv = (char**)malloc(sizeof(char*) * (argc + 5));
  exec_argv[0] = "strace";
  exec_argv[1] = "-T";
  exec_argv[2] = "-xx";
  exec_argv[3] = "-o";
  exec_argv[4] = file_path;
  memcpy(exec_argv + 5, argv + 1, argc * sizeof(char*));
  for(int i = 0; i < argc + 4; ++i)
    Log("%s",exec_argv[i]);

  pid_t pid = fork();
  if(pid < 0) {
    Assert(FONT_BLUE, "Fork failed");
  }

  if(pid == 0) { 
    // close(fildes[0]);
    int blackhole = open("/dev/null", O_RDWR | O_APPEND);
    if(blackhole == -1){ 
      Assert(FONT_RED, "Open /dev/null failed");
    }
    dup2(blackhole, STDOUT_FILENO);
    dup2(blackhole, STDERR_FILENO); 
    // strace must be in some place in the path
    strcpy(exec_path, "strace");
    char *token = strtok(path, ":"); // path can't be used after the operations
    while(execve(exec_path, exec_argv, exec_envp) == -1) {
      memset(exec_path, 0, strlen(exec_path));
      strcpy(exec_path, token);
      strcat(exec_path, "/strace");
      token = strtok(NULL, ":");
      Log("try exec_path: %s",exec_path);
    }
    Assert(FONT_RED, "Should not reach here!");
  } else {
    // close(fildes[1]);
    // dup2(fildes[1], STDIN_FILENO);
    int c = 0;

    
    FILE* f = NULL;
    f = fopen("strace_output", "r");
    if(f == NULL) {
      Assert(FONT_RED, "can't open strace_output");
    }
    while(1) {
      char str[1024]; // 每次从文件内读取一行
      int idx = 0;
      for (idx = 0; c != '\n'; c = fgetc(f), idx++) {
        if (idx >= 1024) {
          Assert(FONT_BLUE, "str is too small");
        }
        if (c == EOF) { idx--; break;}
        else str[idx] = c;
      }
      printf("%s\n", str);
      if(feof(f)) break;
    }

    fclose(f);
  }
}
