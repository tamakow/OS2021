#include <common.h>

static void os_init() {
  pmm->init();
}

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  // pmm->alloc(4);
  for (int i = 0; i < 100000; ++i)
  pmm->alloc(4096);
  Log("After first allocation");
  pmm->alloc(4096);
  Log("After second allocation");
  pmm->alloc(4096);
  Log("After all allocation");
  Log("After one free");
  pmm->alloc(4);
  Log("After 4096 allocation");
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
