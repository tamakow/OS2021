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
    print(FONT_RED, "Invalid arguements!\nUsage: ./sperf-64 [cmd] [args]");
    assert(0);
  }
  //创建子进程，在子进程用strace

}
