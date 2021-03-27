#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>


/* debug */
#define  LIBCO_DEBUG 
#ifdef  LIBCO_DEBUG 

#else

#endif


#define KB         *(1 << 10)
#define STACK_SIZE (4 KB)

enum co_status {
  CO_NEW = 1, 
  CO_RUNNING,
  CO_WAITING,
  CO_DEAD,
};

struct co {
  char *name;
  void (*func)(void *);
  void *arg;

  enum co_status status;
  struct co * waiter;
  jmp_buf context;
  uint8_t  stack[STACK_SIZE];
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  return NULL;
}

void co_wait(struct co *co) {
}

void co_yield() {
}
