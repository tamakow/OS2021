#include <game.h>

#define FPS              30
#define CPS               5
//print GAME OVER
#define CHAR_W            8
#define CHAR_H           16
#define NCHAR           128
// draw the ball
#define SIDE             16
// notice this may cause some problem of float number
#define LEN screen_w / SIDE

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


struct BALL {
  int x, y, v, t;
} ball;

struct BOARD {
  int x, y;
  int len;
} board;

static void video_init();
static void update_board();
// static void new_ball();
// static int min(int a,int b);
// static int randint(int l,int r);


static int screen_w, screen_h;
static uint32_t blank[SIDE * SIDE];
static uint32_t Board[SIDE * SIDE];

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  video_init();

  puts(red"'ESC' to exit this game\n"close);
  puts(green"Please press 'A' or 'D' to move the board\n"close);

  int frame1 = 0, frame2 = 0;
  while (1) {
    frame1 = io_read(AM_TIMER_UPTIME).us/ (1000000 / FPS);
    while(1){
      AM_INPUT_KEYBRD_T event = io_read(AM_INPUT_KEYBRD);
      if (event.keycode == AM_KEY_NONE) break;
      if (event.keydown && event.keycode == AM_KEY_ESCAPE) halt(0);
      if (event.keydown && event.keycode == AM_KEY_A) {
        if(board.x > 0)
          board.x -= 1;
      }
      if (event.keydown && event.keycode == AM_KEY_D) {
        if(board.x + board.len < LEN) 
          board.x += 1;
      }
    }
    if(frame1 > frame2) {
      update_board();
      frame2 = frame1;
    }
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

  for (int i = 0; i < SIDE * SIDE; ++ i) {
    Board[i] = COL_BLUE;
    blank[i] = COL_Cyan;
  }
  //draw the backgroud
  for (int x = 0; x * SIDE <= screen_w; ++ x) {
    for (int y = 0; y * SIDE <= screen_h; ++ y) {
      io_write(AM_GPU_FBDRAW, x*SIDE, y*SIDE, blank, SIDE, SIDE, false);
    }
  }

  //init the board
  board.x = 0; 
  board.y = screen_h - SIDE;
  board.len =  1;
  update_board(); 
}

static void update_board() {
  for (int x = 0; x * SIDE <= screen_w; ++ x) {
    io_write(AM_GPU_FBDRAW, x * SIDE, board.y, blank, SIDE, SIDE, false);
  } 
  for (int x = board.x; x * SIDE <= (board.x + board.len) * SIDE; ++ x) {
    io_write(AM_GPU_FBDRAW, x * SIDE, board.y, Board, SIDE, SIDE, false);
  } 
}


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