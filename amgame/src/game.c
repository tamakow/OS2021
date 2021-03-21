#include <game.h>

#define FPS              30
// draw the ball
#define SIDE             16
// notice this may cause some problem of float number
#define LEN (screen_w / SIDE)

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
  bool exist;
} ball;

struct BOARD {
  int height;
  int head,tail;
} board;

static void video_init();
static void update_screen();
// static void update_state();
static void new_ball();
static void ball_init();
static int min(int a,int b);
static int randint(int l,int r);


static int screen_w, screen_h;
static uint32_t blank[SIDE * SIDE];
static uint32_t Board[SIDE * SIDE];
static uint32_t Ball[SIDE * SIDE];

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  video_init();
  ball_init();
  new_ball();

  puts(red"'ESC' to exit this game\n"close);
  puts(green"Please press 'A' or 'D' to move the board\n"close);

  int frame1 = 0, frame2 = 0;
  while (1) {
    frame1 = io_read(AM_TIMER_UPTIME).us/ (1000000 / FPS);
    // for (; frame1 < frame; ++frame1) {
    //   update_state();
    // }
    while(1){
      AM_INPUT_KEYBRD_T event = io_read(AM_INPUT_KEYBRD);
      if (event.keycode == AM_KEY_NONE) break;
      if (event.keydown && event.keycode == AM_KEY_ESCAPE) halt(0);
      if (event.keydown && event.keycode == AM_KEY_A) {
        if(board.head > 0) {
          board.head -= 1;
          board.tail -= 1;
        }
      }
      if (event.keydown && event.keycode == AM_KEY_D) {
        if(board.tail < LEN){ 
          board.head += 1;
          board.tail += 1;
        }
      }
    }
    if(frame1 > frame2) {
      update_screen();
      frame2 = frame1;
    }
  }
  return 0;
}

static int min(int a, int b) {
  return (a > b)? b : a; 
}

static int randint(int l, int r) {
  return l + (rand() & 0x7fffffff) % (r - l + 1);
}

static void ball_init(){
  ball.exist = false;
  for (int x = 0; x < SIDE; ++ x) {
    for (int y = 0; y < SIDE; ++ y) {
      if (((x - SIDE/2)*(x - SIDE/2) + (y - SIDE/2)*(y - SIDE/2)) <= SIDE*SIDE/4)
        Ball[x * SIDE + y] = COL_RED;
    }
  }
}

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
      io_write(AM_GPU_FBDRAW, x*SIDE, y*SIDE, blank, min(SIDE, screen_w - x * SIDE), min(SIDE, screen_h - y * SIDE), false);
    }
  }

  //init the board
  board.head = (LEN / 2 - 2) > 0 ? (LEN / 2 - 2) : 0; 
  board.height = screen_h - SIDE;
  board.tail = (board.head + 4) < LEN ? board.head + 4 : 1;
  update_screen();
}

static void update_screen() {
  //init the screen
  for(int x = 0; x * SIDE <= screen_h; ++ x)
    io_write(AM_GPU_FBDRAW, x * SIDE, board.height, blank, min(SIDE, screen_w - x * SIDE), min(SIDE, screen_h - board.height), false);
  //update board
  for (int x = board.head; x < board.tail; ++ x) {
    io_write(AM_GPU_FBDRAW, x * SIDE, board.height, Board, min(SIDE, screen_w - x * SIDE), min(SIDE, screen_h - board.height), false);
  } 
  //update ball
  if(ball.exist)
    io_write(AM_GPU_FBDRAW, ball.x * SIDE, ball.y * SIDE, Ball, min(SIDE, screen_w - ball.x * SIDE), min(SIDE, screen_h - ball.y * SIDE), false);
}

static void new_ball() {
  ball.t = 0;
  ball.v = (screen_h - SIDE + 1) / randint(FPS, FPS * 2);
  ball.x = randint (0, screen_w - SIDE);
  ball.y = 0;
  ball.exist = true;
}

// static void update_state() {
//   if(!ball.exist) new_ball();
// }

