#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define DEBUG

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

  //pipe [0 : stdin] [1 : stdout] [2 : stderr]
  int fildes[2]; // 0: read 1: write
  if(pipe(fildes) < 0) {
    Assert(FONT_YELLOW, "Pipe failed");
  }
  Log("%d %d",fildes[0],fildes[1]);

  char *path = getenv("PATH");
  Log("%s",path);

  //创建子进程，在子进程用strace
  pid_t pid = fork();
  if(pid < 0) {
    Assert(FONT_BLUE, "Fork failed");
  }

  if(pid == 0) { 
    
  } else {

  }
}
