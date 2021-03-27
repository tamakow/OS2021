#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define  LIBCO_DEBUG 

#ifdef   LIBCO_DEBUG 
#define Log(format, ...) \
    printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Log(format, ...)
#endif


#define           KB         *(1 << 10)
#define   STACK_SIZE         (4 KB)



enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  char name[32];
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  struct co *    next;
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈
};


struct co* current = NULL;
struct co* list = NULL; // use a list to store coroutines


struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  Log("New Coroutine's name is %s", name);

  struct co *NewCo = (struct co*)malloc(sizeof(struct co));
  
  strcpy(NewCo->name, name);
  memset(NewCo->stack, 0 , sizeof(NewCo->stack));
  NewCo->func   = func;
  NewCo->arg    = arg;
  NewCo->status = CO_NEW;
  NewCo->waiter = NULL;
  NewCo->next   = NULL;

  struct co* walk = list;
  if(walk) {
    while(walk->next) {
      walk = walk->next;
    }
    walk->next = NewCo;
  }else {
    list = NewCo;
  }
  return NewCo;
}

void co_wait(struct co *co) {

}

void co_yield() {
}

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1" : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1" : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
#endif
  );
}


// main is also a coroutine
void __attribute__((constructor)) before_main() {
  list = (struct co*)malloc(sizeof(struct co));
  strcpy(list->name, "main");
  list->next   = NULL;
  list->status = CO_RUNNING;
  memset(list->stack, 0, sizeof(list->stack));
  current = list;
}
