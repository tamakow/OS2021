#include <game.h>

#define FPS              30
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
#define purple "\033[1;35m"
#define close     "\033[0m"


struct BALL {
  int x, y;
  int vx,vy;
  bool exist;
} ball;

struct BOARD {
  int y;
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
static int score = 0;
static uint32_t blank[SIDE * SIDE];
static uint32_t Board[SIDE * SIDE];
static uint32_t Ball[SIDE * SIDE];

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  video_init();
  ball_init();

  puts(red"'ESC' to exit this game\n"close);
  puts(green"Press 'A' or 'D' to move the board\n"close);
  puts(green"Press 'S' to create a new ball\n"close);

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
          board.head -= SIDE;
          board.tail -= SIDE;
        }
      }
      if (event.keydown && event.keycode == AM_KEY_D) {
        if(board.tail < screen_w){ 
          board.head += SIDE;
          board.tail += SIDE;
        }
      }
      if (event.keydown && event.keycode == AM_KEY_S) {
        if(!ball.exist){
          new_ball();
          score = 0;
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
  for (int x = 0; x < screen_w; x += SIDE) {
    for (int y = 0; y < screen_h; y += SIDE) {
      io_write(AM_GPU_FBDRAW, x, y, blank, min(SIDE, screen_w - x), min(SIDE, screen_h - y), false);
    }
  }

  //init the board
  board.head = 0; 
  board.y = screen_h - SIDE;
  board.tail = 8 * SIDE;
  update_screen();
}

static void update_screen() {
  //init the screen
  // notice we need to init all the screen
  // for(int x = 0; x * SIDE <= screen_w; ++ x)
  //   io_write(AM_GPU_FBDRAW, x * SIDE, board.height, blank, min(SIDE, screen_w - x * SIDE), min(SIDE, screen_h - board.height), false);
  for (int x = 0; x < screen_w; x += SIDE) {
    for (int y = 0; y <= screen_h; y += SIDE) {
      io_write(AM_GPU_FBDRAW, x, y, blank, min(SIDE, screen_w - x), min(SIDE, screen_h - y), false);
    }
  }
  //update board
  for (int x = board.head; x < board.tail; x += SIDE) {
    io_write(AM_GPU_FBDRAW, x, board.y, Board, min(SIDE, screen_w - x), min(SIDE, screen_h - board.y), false);
  } 
  //update ball
  if(ball.exist)
    io_write(AM_GPU_FBDRAW, ball.x, ball.y, Ball, min(SIDE, screen_w - ball.x), min(SIDE, screen_h - ball.y), false);
}

static void new_ball() {
  ball.vx = randint(0, SIDE);
  ball.vy = randint(0, SIDE);
  ball.x = randint(0, screen_w - SIDE);
  ball.y = 0;
  ball.exist = true;
}

static void update_state() {
  if(!ball.exist) return;
  //top
  if(ball.vy < 0 && ball.y <= 0){
    ball.vy = -ball.vy;
  }
  //left
  if(ball.vx < 0 && ball.x <= 0) {
    ball.vx = -ball.vx;
  }
  //right
  if(ball.vx > 0 && ball.x >= screen_w - SIDE) {
    ball.vx = -ball.vx;
  }
  if(ball.y + SIDE >= board.y && ball.x  >= board.head  && ball.x  < board.tail && ball.vy > 0) {
    ball.vy = -ball.vy;
    score ++;
    puts(yellow"Nice!\n"close);
  } else if (ball.y + SIDE >= screen_h) {
    puts(red"GAME OVER!\n"close);
    printf(purple"Your score is %d\n"close, score);
    ball.exist = false;
  }
  ball.x += ball.vx;
  ball.y += ball.vy;
  
}

