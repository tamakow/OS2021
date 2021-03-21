#include <game.h>

#define FPS              8
// draw the ball
#define SIDE             16
// notice this may cause some problem of float number
#define LEN (screen_w / SIDE)

#define COL_WHITE  0xeeeeee
#define COL_RED    0xff0000
#define COL_BLUE   0x191970
#define COL_Cyan   0x00ffff


#define false             0
#define true              1

#define red    "\033[1;31m"
#define yellow "\033[1;33m"
#define green  "\033[1;32m"
#define close     "\033[0m"


struct BALL {
  int x, y;
  int vx,vy;
  bool exist;
} ball;

struct BOARD {
  int height;
  int head,tail;
} board;

static void video_init();
static void update_screen();
static void update_state();
static void new_ball();
static void ball_init();
static int min(int a,int b);
int randint(int l,int r);


static int screen_w, screen_h;
static uint32_t blank[SIDE * SIDE];
static uint32_t Board[SIDE * SIDE];
static uint32_t Ball[SIDE * SIDE];

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  video_init();
  ball_init();

  puts(red"'ESC' to exit this game\n"close);
  puts(green"Please press 'A' or 'D' to move the board\n"close);

  int frame1 = 0, frame2 = 0;
  while (1) {
    int frame = io_read(AM_TIMER_UPTIME).us/ (1000000 / FPS);
    for (; frame1 < frame; ++frame1) {
      update_state();
    }
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
      if (event.keydown && event.keycode == AM_KEY_S) {
        new_ball();
      }
    }
    if(frame1 > frame2) {
      update_screen();
      io_write(AM_GPU_FBDRAW, ball.x * SIDE, ball.y * SIDE, Ball, min(SIDE, screen_w - ball.x * SIDE), min(SIDE, screen_h - ball.y * SIDE), false);
      frame2 = frame1;
    }
  }
  return 0;
}

static int min(int a, int b) {
  return (a > b)? b : a; 
}

int randint(int l, int r) {
  return l + (rand() & 0x7fffffff) % (r - l + 1);
}

static void ball_init(){
  ball.exist = false;
  for (int x = 0; x < SIDE; ++ x) {
    for (int y = 0; y < SIDE; ++ y) {
      if (((x - SIDE/2)*(x - SIDE/2) + (y - SIDE/2)*(y - SIDE/2)) <= SIDE*SIDE/4)
        Ball[x * SIDE + y] = COL_RED;
      else Ball[x * SIDE + y] = COL_Cyan;
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
  board.head = 0;//(LEN / 2 - 2) > 0 ? (LEN / 2 - 2) : 0; 
  board.height = screen_h - SIDE;
  board.tail = LEN -1;//(board.head + 4) < LEN ? board.head + 4 : 1;
  update_screen();
}

static void update_screen() {
  //init the screen
  // notice we need to init all the screen
  // for(int x = 0; x * SIDE <= screen_w; ++ x)
  //   io_write(AM_GPU_FBDRAW, x * SIDE, board.height, blank, min(SIDE, screen_w - x * SIDE), min(SIDE, screen_h - board.height), false);
  for (int x = 0; x * SIDE <= screen_w; ++ x) {
    for (int y = 0; y * SIDE <= screen_h; ++ y) {
      io_write(AM_GPU_FBDRAW, x*SIDE, y*SIDE, blank, min(SIDE, screen_w - x * SIDE), min(SIDE, screen_h - y * SIDE), false);
    }
  }
  //update board
  for (int x = board.head; x < board.tail; ++ x) {
    io_write(AM_GPU_FBDRAW, x * SIDE, board.height, Board, min(SIDE, screen_w - x * SIDE), min(SIDE, screen_h - board.height), false);
  } 
  //update ball
  if(ball.exist)
    io_write(AM_GPU_FBDRAW, ball.x * SIDE, ball.y * SIDE, Ball, min(SIDE, screen_w - ball.x * SIDE), min(SIDE, screen_h - ball.y * SIDE), false);
}

static void new_ball() {
  ball.vx = screen_h / FPS;
  ball.vy = screen_w / FPS;
  ball.x = randint(0, LEN);
  ball.y = 0;
  ball.exist = true;
}

static void update_state() {
  if(!ball.exist) return;
  //top
  if(ball.vy < 0 && ball.y * SIDE == 0){
    ball.vy = -ball.vy;
  }
  //left
  if(ball.vx < 0 && ball.x * SIDE == 0) {
    ball.vx = -ball.vx;
  }
  //right
  if(ball.vx > 0 && ball.x * SIDE == screen_w - SIDE) {
    ball.vx = -ball.vx;
  }
  if(ball.y * SIDE >= board.height && ball.x  >= board.head  && ball.x  < board.tail) {
    ball.vy = -ball.vy;
    puts(yellow"Nice!\n"close);
  }
  //bottom
  else {
    puts(red"GAME OVER!\n"close);
    ball.exist = false;
  }
  ball.x += ball.vx/SIDE;
  ball.y += ball.vy/SIDE;
  
}

