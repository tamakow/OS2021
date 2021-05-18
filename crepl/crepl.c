#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdint.h>

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
#define Assert(format, ...)\
    Log(format,##  __VA_ARGS__);\
    assert(0)
#else
#define Assert(format, ...)\
    assert(0)
#endif


static char line[4096];
static const char _func[] = "int";
static bool func = false;

int main(int argc, char *argv[]) {
  while (1) {
    printf(">> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    func = (strncmp(line, _func, 3) == 0);
    if(func)
      print(FONT_GREEN, "this expression is function");
    else
      print(FONT_GREEN, "this expression is not function");
  }
}
