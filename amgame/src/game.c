#include <game.h>


#define FPS            30
#define CPS             5
//print GAME OVER
#define CHAR_W          8
#define CHAR_H         16
#define NCHAR         128
#define COL_WHITE    0xeeeeee
#define COL_RED      0xdc143c
#define COL_BLUE     0x191970

// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}
