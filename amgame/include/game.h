#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>


#define FPS            30
#define CPS             5
//print GAME OVER
#define CHAR_W          8
#define CHAR_H         16
#define NCHAR         128
#define COL_WHITE    0xeeeeee
#define COL_RED      0xdc143c
#define COL_BLUE     0x8a2be2

void splash();
void print_key();
static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}
