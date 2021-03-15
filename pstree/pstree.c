#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>

#define PROC_BASE "/proc"
#define COMM_LEN 64

// struct definition
typedef struct proc {
  pid_t pid;
  char comm[COMM_LEN + 2];
  char state;
  pid_t ppid;
  struct proc *parent;
  struct proc *next;
  struct child *children;
} PROC;

struct child {
  PROC *children;
  struct child *next;
} CHILD;

struct option options[] = {
  {"show-pids", 0, NULL, 'p'},
  {"numeric-sort", 0, NULL, 'n'},
  {"version", 0, NULL, 'V'},
  {0, 0, 0, 0}
};

// function definition
static void usage();
void print_version();
static void read_stat(int pid);
static void read_proc();

// global variable definition
PROC* list = NULL; // 


int main(int argc, char *argv[]) {
  int c;

  for (int i = 1; i < argc; i++) {
    assert(argv[i]);
    // printf("argv[%d] = %s\n", i, argv[i]);
    while((c = getopt_long(argc,argv,"pnV", options, NULL)) != -1) {
      switch (c) {
        case 'V':
          print_version();
          return 0;
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
  read_proc();

  assert(!argv[argc]);
  return 0;
}


static void usage(){
  printf("\033[31mPlease input valid arguments\033[0m \n"
  "Usage: pstree [-p, --show-pids] [-n, --numeric-sort] [-V, --version]\n");
  exit(1);
}

void print_version(){
  printf("pstree 1.0\n"
         "Copyright (C) 2021-2021 Tamakow\n");
}

static void read_proc(){
  // read /proc 
  DIR *dir_ptr;
  pid_t pid;
  char *endptr;
  struct dirent *direntp;
  if (!(dir_ptr = opendir(PROC_BASE))) {
    fprintf(stderr, ("Can't open /proc"));
    exit(1);
  }
  fprintf(stderr, "\033[34mSuccessfully open /proc\033[01m\n");
  // Test for reading filenames in /proc
  // FILE* fp;
  // fp = fopen("1.txt", "w+");
  // while((direntp = readdir(dir_ptr)) != NULL) {
  //   fprintf(fp, "%s", direntp->d_name);
  //   fprintf(fp, "\n");
  // }
  // fclose(fp);
  while ((direntp = readdir(dir_ptr)) != NULL) {
    //read the process information in /proc and attach it to the tree 
    pid = (pid_t) strtol(direntp->d_name,&endptr,10);
    if(endptr != direntp->d_name && endptr[0] == '\0') {
      //judge the relationship and add to the root
      read_stat(pid);
    }
  }
}

static void read_stat (int pid) {
   // read /proc/[pid]/stat 
   FILE* fp;
   char path[64];
   char comm[COMM_LEN + 2];
   char state;
   int ppid;

   sprintf(path, "%s/%d/stat", PROC_BASE, pid);
   if((fp = fopen(path, 'r')) != NULL) {
     fscanf(fp, "%d (%16s) %c %d",&pid,comm,&state,&ppid);
     printf("%s\n",comm);
     fclose(fp);
   }
}