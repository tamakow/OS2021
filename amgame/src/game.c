#include <game.h>

#define FPS              30
#define CPS               5
//print GAME OVER
#define CHAR_W            8
#define CHAR_H           16
#define NCHAR           128
// draw the ball
#define SIDE             16
#define COL_WHITE  0xeeeeee
#define COL_RED    0xdc143c
#define COL_BLUE   0x191970
#define COL_Cyan   0x00ffff


#define false             0
#define true              1

#define red    "\033[1;31m"
#define yellow "\033[1;33m"
#define green  "\033[1;32m"
#define close     "\033[0m"

struct character {
  int x, y, v, t;
} ball;


static void video_init();
// static void new_ball();
// static int min(int a,int b);
// static int randint(int l,int r);


static int screen_w, screen_h;
static uint32_t blank[SIDE * SIDE];

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  video_init();

  // puts("mainargs = \"");
  // puts(args); // make run mainargs=xxx
  // puts("\"\n");

  // splash();

  puts(red"'ESC' to exit this game\n"close);
  puts(green"Please press 'A' or 'D' to move the board\n"close);


  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}

// static int min(int a, int b) {
//   return (a > b)? b : a; 
// }

// static randint(int l, int r) {
//   return l + (rand() & 0x7fffffff) % (r - l + 1);
// }

static void video_init() {
  screen_h = io_read(AM_GPU_CONFIG).height;
  screen_w = io_read(AM_GPU_CONFIG).width;

  //draw a screen of balls 
  // uint32_t pixels[SIDE * SIDE];
  // for (int x = 0; x < SIDE; ++ x) {
  //   for (int y = 0; y < SIDE; ++ y) {
  //     if (((x - SIDE/2)*(x - SIDE/2) + (y - SIDE/2)*(y - SIDE/2)) <= SIDE*SIDE/4)
  //       pixels[x*SIDE + y] = COL_WHITE;
  //   }
  // }
  // for (int x = 0; x * SIDE <= screen_h; ++ x) {
  //   for (int y = 0; y * SIDE <= screen_w; ++ y) {
  //     io_write(AM_GPU_FBDRAW, x*SIDE, y*SIDE, pixels, SIDE, SIDE, false);
  //   }
  // }


  uint32_t pixels[SIDE * SIDE];
  for (int i = 0; i < SIDE * SIDE; ++ i) {
    pixels[i] = COL_BLUE;
    blank[i] = COL_Cyan;
  }
  for (int x = 0; x * SIDE <= screen_w; ++ x) {
    for (int y = 0; y * SIDE <= screen_h; ++ y) {
      io_write(AM_GPU_FBDRAW, x*SIDE, y*SIDE, blank, SIDE, SIDE, false);
    }
  }

  for (int x = 3* (screen_w / 8) / SIDE; x * SIDE <= 5 * (screen_w / 8); ++ x) {
    io_write(AM_GPU_FBDRAW, x * SIDE, screen_h - SIDE, pixels, SIDE, SIDE, false);
  }  
}
