#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>

#define PROC_BASE "/proc"

// struct definition
struct option options[] = {
  {"show-pids", 0, NULL, 'p'},
  {"numeric-sort", 0, NULL, 'n'},
  {"version", 0, NULL, 'V'},
  {0, 0, 0, 0}
};

// function definition
void usage();
void print_version();
void read_proc();

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
  //读取proc所有目录到全局变量上
  read_proc();

  assert(!argv[argc]);
  return 0;
}


void usage(){
  printf("\033[31mPlease input valid arguments\033[0m \n"
  "Usage: pstree [-p, --show-pids] [-n, --numeric-sort] [-V, --version]\n");
  exit(1);
}

void print_version(){
  fprintf(stderr,_("pstree 1.0\n"
                   "Copyright (C) 2021-2021 Tamakow\n"));
}

void read_proc(){
  DIR *dir_ptr;
  struct dirent *direntp;
  if (!(dir_ptr = opendir(PROC_BASE))) {
    fprintf(stderr, _("Can't open /proc"));
    exit(1);
  }
}