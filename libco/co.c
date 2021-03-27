#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#define          KiB         *(1 << 10)
#define   STACK_SIZE         (64 KiB)


#define          red         "\033[1;31m"
#define       yellow         "\033[1;33m"
#define        green         "\033[1;32m"
#define       purple         "\033[1;35m"
#define        done          "\033[0m"

#define  LIBCO_DEBUG 

#ifdef   LIBCO_DEBUG 
#define Log(format, ...) \
    printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Log(format, ...)
#endif


enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  const char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  struct co *    next;
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE] __attribute__ ((aligned(16))); // 协程的堆栈
  void      *    stackptr;
};


struct co* current = NULL;
struct co* list = NULL; // use a list to store coroutines
int cnt = 0;


inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1" : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1" : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
#endif
  );
}

void free_co(struct co* co) {
  if(list == NULL) return;
  struct co *walk = list;
  if(list == co) {
    walk = list->next;
    free(list);
    list = walk;
  }
  while(walk->next) {
    if(walk->next == co) {
      struct co* tmp = walk->next->next;
      free(walk->next);
      walk->next = tmp;
      break;
    }
    walk = walk->next;
  }
}

struct co* RandomChooseCo () {
  int rd;
  struct co* ret;
  label:
  rd = rand() % cnt;
  ret = list;

  while(rd--) ret = ret->next;

  if(ret->status != CO_RUNNING && ret->status != CO_NEW) 
    goto label;
  return ret;
}

void Entry(struct co* co) {
  Log("Now in %s 's entry",current->name);
  co->status = CO_RUNNING;
  co->func(co->arg);
  
  //finished
  co->status = CO_DEAD;
  if(co->waiter) co->waiter->status = CO_RUNNING;
  co_yield();
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  Log("New Coroutine's name is "red"%s"done, name);

  struct co *NewCo = NULL;
  NewCo = (struct co*)malloc(sizeof(struct co));

  memset(NewCo->stack, 0 , sizeof(NewCo->stack));
  NewCo->stackptr = (void *)((((uintptr_t)NewCo->stack+sizeof(NewCo->stack))>>4)<<4);
  NewCo->func     = func;
  NewCo->arg      = arg;
  NewCo->status   = CO_NEW;
  NewCo->waiter   = NULL;
  NewCo->next     = NULL;
  NewCo->name     = name;

  cnt ++;
  struct co* walk = list;
  while(walk->next) {
      walk = walk->next;
  }
  walk->next = NewCo;
  return NewCo;
}


//buggy
void co_yield() {
  Log("Now in co_yield");
  int val = setjmp(current->context);
  if(val == 0) {
    current = RandomChooseCo();
    Log("current co is %s %d",current->name,current->status);
    if(current->status == CO_NEW) {
      Log("current hasn't run yet");
      // stack_switch_call(current->stackptr, Entry, (uintptr_t)current);
      #if __x86_64__
        asm volatile("mov %0, %%rsp": : "b"((uintptr_t)current->stackptr));
        Entry(current);
      #else
        stack_switch_call(current->stackptr, Entry, (uintptr_t)current);
      #endif
    }else {
      longjmp(current->context, 0);
    }
  }else {
    return;
  }
}

void co_wait(struct co *co) {
  Log("Waiting Coroutine "red"%s"done, co->name);
  if(co->status == CO_DEAD) {
    Log("free %s",co->name);
    free_co(co);
    cnt--;
    co->waiter->status = CO_RUNNING;
    return;
  }
  co->waiter = current;
  current->status = CO_WAITING;
  // must run co and free it after it finishes
  co_yield();
}


// main is also a coroutine
void __attribute__((constructor)) before_main() {
  list = (struct co*)malloc(sizeof(struct co));
  list->name   = "main";
  list->next   = NULL;
  list->waiter = NULL;
  list->status = CO_RUNNING;
  list->stackptr = (void *)((((uintptr_t)list->stack+sizeof(list->stack))>>4)<<4);
  memset(list->stack, 0, sizeof(list->stack));
  cnt = 1;
  current = list;
  srand((unsigned int)time(NULL));
}
