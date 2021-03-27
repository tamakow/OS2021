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
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈
  void      *    stackptr;
};


struct co* current = NULL;
struct co* list = NULL; // use a list to store coroutines
static int cnt = 0;
struct co* WaitCo  = NULL; //NULL means randomly choose the next running co, while op = 1 means in the co_wait stat


inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1" : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1" : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
#endif
  );
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
  if(co->status == CO_DEAD) {
    Log("free %s",co->name);
    free_co(co);
    cnt--;
    return;
  }
  Log("Waiting Coroutine "red"%s"done, co->name);
  current->waiter = co;
  current->status = CO_WAITING;
  WaitCo = co;
  // must run co and free it after it finishes
  co_yield();
  current->waiter = NULL;
  // current->status = CO_RUNNING;
}

void co_yield() {
  Log("Now in co_yield");
  int val = setjmp(current->context);
  if(val == 0) {
    current = RandomChooseCo();
    Log("current co is %s %d",current->name,current->status);
    if(current->status == CO_NEW) {
      Log("current hasn't run yet");
      current->status = CO_RUNNING;
      stack_switch_call(current->stackptr, entry, (uintptr_t)NULL);
    }else {
      longjmp(current->context, 1);
    }
  }else {
    current->status = CO_RUNNING;
    return;
  }
}


// main is also a coroutine
void __attribute__((constructor)) before_main() {
  list = (struct co*)malloc(sizeof(struct co));
  list->name   = "main";
  list->next   = NULL;
  list->status = CO_RUNNING;
  memset(list->stack, 0, sizeof(list->stack));
  cnt ++;
  current = list;
  srand((unsigned int)time(NULL));
}

struct co* RandomChooseCo () {
  struct co* ret = list;
  if(!WaitCo) {
    int rd = rand() % cnt;
    while(rd--) {
      ret = ret->next;
    }
  } else {
    while(ret != WaitCo) {
      ret = ret->next;
    }
    WaitCo = NULL;
  }
  
  // element in the list can't be CO_DEAD but ret->waiter may be finished thus its status becomes CO_DEAD
  while(ret->status == CO_WAITING) {
    if(ret->waiter == NULL || ret->waiter->status == CO_DEAD) break;
    else ret = ret->waiter;
  }

  return ret;
}

void *entry() {
  Log("Now in %s 's entry",current->name);
  current->status = CO_RUNNING;
  current->func(current->arg);
  
  //finished
  current->status = CO_DEAD;

  co_yield();
  return 0;
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