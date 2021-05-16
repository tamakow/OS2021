#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define DEBUG

#include "debug.h"

int main(int argc, char *argv[]) {
  if(argc < 2) {
    print(FONT_RED, "Invalid arguements!\nUsage: ./sperf-64 [cmd] [args]");
    assert(0);
  }
}
