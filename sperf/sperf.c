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

#define  FONT_BLACK          "\033[1;30m"
#define  FONT_RED            "\033[1;31m"
#define  FONT_GREEN          "\033[1;32m"
#define  FONT_YELLOW         "\033[1;33m"
#define  FONT_BLUE           "\033[1;34m"
#define  FONT_PURPLE         "\033[1;35m"
#define  FONT_CYAN           "\033[1;36m"
#define  FONT_WHITE          "\033[1;37m"

#define  FONT_RESET          "\033[0m"

#ifdef  DEBUG 
#define Log(format, ...) \
    printf(FONT_BLUE "[%s,%d,%s] " format FONT_RESET"\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#define print(FONT_COLOR, format, ...) \
    printf(FONT_COLOR format FONT_RESET "\n", \
        ## __VA_ARGS__)
#else
#define Log(format, ...)
#define print(FONT_COLOR, format, ...)
#endif


#ifdef  DEBUG
#define Assert(color, format, ...)\
    print(color, format,##  __VA_ARGS__);\
    assert(0)
#else
#define Assert(color, format, ...)\
    assert(0)
#endif


#define NAME_LEN 64
#define TIME_LEN 64

typedef struct syscall_node {
  char   name[NAME_LEN];
  double time;
  struct syscall_node *next;
} syscall_node_t;

syscall_node_t *head;
double total_time = 0;

void bubble_sort() {
  if(head == NULL || head->next == NULL) return;
  for(syscall_node_t *i = head; i != NULL; i = i->next) {
    for(syscall_node_t *j = i->next; j != NULL; j = j->next) {
      if(i->time < j->time) {
        char tmp_name[NAME_LEN];
        strcpy(tmp_name,i->name);
        double tmp_time = i->time;
        strcpy(i->name, j->name);
        i->time = j->time;
        strcpy(j->name, tmp_name);
        j->time = tmp_time;
      }
    }
  }
}

void display() {
  bubble_sort();
  syscall_node_t *walk = head;
  for(int i = 0;i < 5 && walk != NULL;i++, walk = walk->next) {
    printf("%.24s (%2lf%%)\n",walk->name, (walk->time * 100) / total_time);
  }
  printf("====================\n");
  for(int i = 0; i < 80; ++i)
    putc('\0', stdout);
  fflush(stdout);
}


int main(int argc, char *argv[]) {
  if(argc < 2) {
    Assert(FONT_RED, "Invalid arguements!\nUsage: ./sperf-64 [cmd] [args]");
  }

  //正则
  regex_t name_preg;
  regex_t time_preg;
  if(regcomp(&name_preg, "^[a-zA-Z_\\*0-9]*?\\(", REG_EXTENDED) !=  0) {
    Assert(FONT_BLUE,"Name regcomp failed!");
  }
  if(regcomp(&time_preg, "<[0-9\\.]*>$", REG_EXTENDED) !=  0) {
    Assert(FONT_BLUE,"time regcomp failed!");
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
  exec_argv = (char**)malloc(sizeof(char*) * (argc + 4));
  exec_argv[0] = "strace";
  exec_argv[1] = "-T";
  exec_argv[2] = "-o";
  exec_argv[3] = file_path;
  memcpy(exec_argv + 4, argv + 1, argc * sizeof(char*));
  for(int i = 0; i < argc + 3; ++i)
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

    clock_t l = clock();
    while(1) {
      clock_t r = clock();
      if(r - l >= CLOCKS_PER_SEC) {
        display();
        l = r;
      }
      char str[1024]; // 每次从文件内读取一行
      int idx = 0;
      c = fgetc(f);
      for (idx = 0; c != '\n'; c = fgetc(f), idx++) {
        if (idx >= 1024) {
          Assert(FONT_BLUE, "str is too small");
        }
        if (c == EOF)  break;
        else str[idx] = c;
      }
      str[idx] = '\0';

      regmatch_t name_match;
      char name[NAME_LEN];
      regmatch_t time_match;
      char time[NAME_LEN];
      double _time = 0;

      //读取系统调用名称
      if(regexec(&name_preg, str, 1, &name_match, 0) == REG_NOMATCH) {
        continue;
        // Assert(FONT_BLUE, "No match for name");
      }
      strncpy(name, str + name_match.rm_so, name_match.rm_eo - name_match.rm_so);
      name[name_match.rm_eo - name_match.rm_so - 1] = '\0';
      if(strcmp(name, "exit_group") == 0) break;
      //读取系统调用时间
      Log("name is %s",str);
      if(regexec(&time_preg, str, 1, &time_match, 0) == REG_NOMATCH) {
        continue;
        // Assert(FONT_BLUE, "No match for time");
      }
      strncpy(time, str + time_match.rm_so + 1, time_match.rm_eo - time_match.rm_so - 1);
      time[time_match.rm_eo - time_match.rm_so - 2] = '\0';
      _time = atof(time); 

      //更新syscall_node_t链表
      if(head == NULL) {
        head = (syscall_node_t*)malloc(sizeof(syscall_node_t));
        strcpy(head->name, name);
        head->time = _time;
        head->next = NULL;
      } else {
        syscall_node_t *p = head;
        syscall_node_t *q = NULL;
        while ((p != NULL) && (strcmp(p->name, name) != 0)){
          q = p;
          p = p->next;
        }
        if(p == NULL) {
          syscall_node_t* new_node = (syscall_node_t*)malloc(sizeof(syscall_node_t));
          strcpy(new_node->name, name);
          new_node->time = _time;
          new_node->next = NULL;
          q->next = new_node;
        } else {
          p->time += _time;
        }
      }
      total_time += _time;
      if(feof(f)) break;
    }
    display();
    fclose(f);
  }
  regfree(&name_preg);
  regfree(&time_preg);
  return 0;
}
