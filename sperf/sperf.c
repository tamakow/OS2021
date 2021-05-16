#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "debug.h"

#define DEBUG

int main(int argc, char *argv[]) {
  if(argc < 2) {
    print(FONT_RED, "Invalid arguements! Usage: ./sperf-64 [cmd] [args]");
    assert(0);
  }
}
