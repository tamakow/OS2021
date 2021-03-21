#include <game.h>

#define FPS             30
#define CPS              5
//print GAME OVER
#define CHAR_W           8
#define CHAR_H          16
#define NCHAR          128
// draw the ball
#define SIDE            16
#define COL_WHITE 0xeeeeee
#define COL_RED   0xdc143c
#define COL_BLUE  0x191970


#define false            0
#define true             1

static void video_init();

static int screen_w, screen_h;

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  video_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  // splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}

static void video_init() {
  screen_h = io_read(AM_GPU_CONFIG).height;
  screen_w = io_read(AM_GPU_CONFIG).width;

  uint32_t pixels[SIDE * SIDE];
  for (int x = 0; x < SIDE; ++ x) {
    for (int y = 0; y < SIDE; ++ y) {
      if (((x - SIDE/2)*(x - SIDE/2) + (y - SIDE/2)*(y - SIDE/2)) <= SIDE*SIDE/4)
        pixels[x*SIDE + y] = COL_WHITE;
    }
  }
  for (int x = 0; x * SIDE <= screen_h; ++ x) {
    for (int y = 0; y * SIDE <= screen_w; ++ y) {
      io_write(AM_GPU_FBDRAW, x*SIDE, y*SIDE, pixels, SIDE, SIDE, false);
    }
  }
}
