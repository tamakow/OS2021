#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/wait.h>


#define bool uint8_t
#define false 0
#define true 1

// #define DEBUG

#define  FONT_BLACK          "\033[1;30m"
#define  FONT_RED            "\033[1;31m"
#define  FONT_GREEN          "\033[1;32m"
#define  FONT_YELLOW         "\033[1;33m"
#define  FONT_BLUE           "\033[1;34m"
#define  FONT_PURPLE         "\033[1;35m"
#define  FONT_CYAN           "\033[1;36m"
#define  FONT_WHITE          "\033[1;37m"

#define  FONT_RESET          "\033[0m"

#define print(FONT_COLOR, format, ...) \
    printf(FONT_COLOR format FONT_RESET "\n", \
        ## __VA_ARGS__)

#ifdef  DEBUG 
#define Log(format, ...) \
    printf(FONT_BLUE "[%s,%d,%s] " format FONT_RESET"\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Log(format, ...)
#endif

#ifdef  DEBUG
  #define Assert(cond, format, ...) \
    do { \
      if (!(cond)) { \
        Log(format, ## __VA_ARGS__); \
        exit(1); \
      } \
    } while (0)
#else
#define Assert(cond, format, ...)\
    do { \
      if (!(cond)) { \
        exit(1); \
      } \
    } while (0)
#endif


static char line[4096];
const char _func[] = "int";
bool func = false;
static int cnt = 0;



int main(int argc, char *argv[]) {
  while (1) {
    printf(">> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    if(line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';
    // if(strcmp(line, "exit") == 0) break;
    func = (strncmp(line, _func, 3) == 0);

    char tmp_file_1[] = "/tmp/tmp_c_XXXXXX";
    char tmp_file_2[] = "/tmp/tmp_so_XXXXXX";
    // int fd_c = 0, fd_so = 0;
    Assert((mkstemp(tmp_file_1)) != -1, "create tmp_c_file failed!");
    Assert((mkstemp(tmp_file_2)) != -1, "create tmp_so_file failed! ");

    char tmp_c_file[4096],tmp_so_file[4096];
    sprintf(tmp_c_file, "%s.c", tmp_file_1);
    sprintf(tmp_so_file, "%s.so", tmp_file_2);
    Assert(rename(tmp_file_1, tmp_c_file) != -1, "rename tmp_c_file failed");
    Assert(rename(tmp_file_2, tmp_so_file) != -1, "rename tmp_so_file failed");

    Log("%s %s",tmp_c_file, tmp_so_file);
    // write(fd_c, line, sizeof(line));
    FILE* file_c = fopen(tmp_c_file, "w+");
    if(func)
      fprintf(file_c, "%s", line);
    else
      fprintf(file_c, "__expr_wrapper_%d() {return (%s);}", cnt, line);
    fclose(file_c);

    #if defined(__x86_64__)
      char *exec_argv[] = {"gcc", "-m64","-Wno-implicit-function-declaration", "-w", "-fPIC", "-shared", "-o", tmp_so_file, tmp_c_file, NULL};
    #elif defined(__i386__)
      char *exec_argv[] = {"gcc", "-m32","-Wno-implicit-function-declaration", "-w", "-fPIC", "-shared", "-o", tmp_so_file, tmp_c_file, NULL};
    #endif

    int pid = fork();
    Assert(pid != -1, "fork failed!");

    if(pid == 0) {
      int blackhole = open("/dev/null", O_RDWR);
      Assert(blackhole != -1, "Open /dev/null failed");
      dup2(blackhole, STDOUT_FILENO);
      dup2(blackhole, STDERR_FILENO); 
      execvp(exec_argv[0], exec_argv);
    } else {
      int status = 0;
      wait(&status);
      if(WEXITSTATUS(status) != 0)
        print(FONT_RED, "Compile Error");
      else {
        void *e = dlopen(tmp_so_file, RTLD_GLOBAL | RTLD_NOW);
        if(e == NULL) {
          print(FONT_RED, "Load failed: %s",dlerror());
        } else {
          if(func) print(FONT_YELLOW, "OK.");
          else {
            char wrapper[128];
            sprintf(wrapper, "__expr_wrapper_%d",cnt++);
            int (*entry)();
            entry = dlsym(e, wrapper); 
            char *error = dlerror();
            if(error) {
              print(FONT_BLUE, "dlsym failed: %s", error);
            }
            else
              print(FONT_GREEN, "= %d.", entry());
            dlclose(e);
          }
        }
      }
    }
  }
}
