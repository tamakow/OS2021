#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>




int main(int argc, char *argv[]) {
  static char line[4096];
  while (1) {
    printf(">> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      
      break;
    }
    printf("Got %zu chars.\n", strlen(line)); // ??
  }
}
