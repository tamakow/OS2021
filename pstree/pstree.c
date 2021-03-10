#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>

struct option options[] = {
  {"show-pids", 0, NULL, 'p'},
  {"numeric-sort", 0, NULL, 'n'},
  {"version", 0, NULL, 'V'},
  {0, 0, 0, 0}
};

void usage(){
  printf("\033[31mPlease input valid arguments\033[0m \n"
  " Usage: pstree [-p, --show-pids] [-n, --numeric-sort] [-V, --version]\n");
  exit(1);
}

void print_version(){
  printf("\033[32m successfully identify -V --version\033[01m \n");
}

int main(int argc, char *argv[]) {
  int c;

  for (int i = 1; i < argc; i++) {
    assert(argv[i]);
    // printf("argv[%d] = %s\n", i, argv[i]);
    while((c = getopt_long(argc,argv,"pnV", options, NULL)) != -1) {
      switch (c) {
        case 'V':
          print_version();
          break;
        case 'p':
          printf("\033[31m successfully identify -p --show-pids\033[01m \n");
          break;
        case 'n':
          printf("\033[31m successfully identify -n --numeric-sort\033[01m \n");
          break;
        default: usage();
      }
    }
  }
  assert(!argv[argc]);
  return 0;
}
