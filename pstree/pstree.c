#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>

#define PROC_BASE "/proc"
#define COMM_LEN 64

/* struct definition */
typedef struct proc {
  pid_t pid;
  char comm[COMM_LEN + 2];
  char state;
  pid_t ppid;
  struct proc *parent;
  struct proc *next;
  struct proc *child;
} PROC;

struct option options[] = {
  {"show-pids", 0, NULL, 'p'},
  {"numeric-sort", 0, NULL, 'n'},
  {"version", 0, NULL, 'V'},
  {0, 0, 0, 0}
};

/* function definition */
static void usage();
void print_version();
static PROC *find_process (pid_t pid, PROC* pre);
static void add_process (pid_t pid, char* comm, char state, pid_t ppid);
static void read_stat(int pid);
static void read_proc();
static void print_tree(PROC *pre);

/* global variable definition */
/* bugs here */
/* systemd's state may not be S */
/* state doesn't matter in this lab*/
PROC list = {1, "systemd", 'S', -1, NULL, NULL, NULL}; // use a list to record the relation among processes
static int show_pid = 0;
static int numeric_sort = 0;


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
          // printf("\033[31m successfully identify -p --show-pids\033[0m \n");
          show_pid = 1;
          break;
        case 'n':
          // printf("\033[31m successfully identify -n --numeric-sort\033[0m \n");
          numeric_sort = 1;
          break;
        default: usage();
      }
    }
  }
  read_proc();
  print_tree(&list);
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


/* read /proc */
static void read_proc(){ 
  DIR *dir_ptr;
  pid_t pid;
  char *endptr;
  struct dirent *direntp;
  if (!(dir_ptr = opendir(PROC_BASE))) {
    fprintf(stderr, ("Can't open /proc"));
    exit(1);
  }
  // fprintf(stderr, "\033[34mSuccessfully open /proc\033[01m\n");
  while ((direntp = readdir(dir_ptr)) != NULL) {
    /* read the process information in /proc  */ 
    pid = (pid_t) strtol(direntp->d_name,&endptr,10);
    if(endptr != direntp->d_name && endptr[0] == '\0') {
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
   pid_t ppid;

   sprintf(path, "%s/%d/stat", PROC_BASE, pid);
   if((fp = fopen(path, "r")) != NULL) {
     fscanf(fp, "%d (%[^)]) %c %d",&pid,comm,&state,&ppid);
     if(comm[0] == '(') {
       int len = strlen(comm);
       comm[len] = ')';
     } 
     add_process(pid, comm, state, ppid);
     fclose(fp);
   }
}


static void add_process (pid_t pid, char* comm, char state, pid_t ppid) {
  PROC* new_proc = (PROC*)malloc(sizeof(PROC));
  strncpy(new_proc->comm,comm,COMM_LEN+2);
  new_proc->pid=pid;
  new_proc->ppid=ppid;
  new_proc->state=state;
  new_proc->parent = new_proc->next = NULL;
  new_proc->child=NULL;

  //first find whether this process has been in the list 
  PROC *tmp = find_process(pid, &list);
  if(tmp){ return; } 

  //find new process 's parent
  PROC *parent = find_process(ppid, &list);
  //if parent is not in the list, assert
  //how to guarantee parent added before child ? 
  // assert(!parent);
  if(!parent) return;


  new_proc->parent = parent;
  new_proc->next = parent->child;
  parent->child = new_proc;
  
}

static PROC *find_process (pid_t pid, PROC* pre) {
  PROC *walk = pre;

  if (pid == walk->pid) return walk;

  PROC* exist;
  if (walk->next) {
    exist = find_process(pid,walk->next);
    if(exist) return exist;
  }
  if(walk->child) {
    exist = find_process(pid,walk->child);
    if(exist) return exist;
  }

  return NULL;
}

static int Tab = 0;
static void print_tree (PROC* pre) {
  for(int i = 0; i < Tab; ++i)
    printf(" ");
  printf("%s\n",pre->comm);
  if(pre->child) {
    Tab += 6;
    print_tree(pre->child);
    Tab -= 6;
  }
  if(pre->next) {
    print_tree(pre->next);
  }
}