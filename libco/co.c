#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define          KiB         *(1 << 10)
#define   STACK_SIZE         (64 KiB)


#define          red         "\033[1;31m"
#define       yellow         "\033[1;33m"
#define        green         "\033[1;32m"
#define       purple         "\033[1;35m"
#define        done          "\033[0m"

// #define  LIBCO_DEBUG 

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


void free_co(struct co* co) {
  if(list == NULL) return;
  struct co *walk = list;
  if(list == co) {
    walk = list->next;
    if(walk == list) walk = NULL;
    free(list);
    list = walk;
    return;
  }
  while(walk->next != list && walk->next != co) {
    walk = walk->next;
  }
  walk->next = co->next;
}

struct co* CircularChooseCo () {
  struct co* ret = current->next;
  while(ret->status != CO_RUNNING && ret->status != CO_NEW)
    ret = ret->next;
  return ret;
}

void Entry(struct co* co) {
  Log("Now in %s 's entry",current->name);
  co->status = CO_RUNNING;
  co->func(co->arg);
  
  co->status = CO_DEAD;
  if(co->waiter) co->waiter->status = CO_RUNNING;
  co_yield();
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *NewCo = NULL;
  NewCo = (struct co*)malloc(sizeof(struct co));

  memset(NewCo->stack, 0 , sizeof(NewCo->stack));
  NewCo->stackptr = (void *)((((uintptr_t)NewCo->stack+sizeof(NewCo->stack))>>4)<<4);
  NewCo->func     = func;
  NewCo->arg      = arg;
  NewCo->status   = CO_NEW;
  NewCo->waiter   = NULL;
  NewCo->next     = list;
  NewCo->name     = name;

  struct co* walk = list;
  while(walk->next!=list) {
      walk = walk->next;
  }
  walk->next = NewCo;
  return NewCo;
}

void co_yield() {
  int val = setjmp(current->context);
  if(val == 0) {
    current = CircularChooseCo();
    if(current->status == CO_NEW) {
      #if __x86_64__
        asm volatile("mov %0, %%rsp": : "b"((uintptr_t)current->stackptr));
      #else
        asm volatile("mov %0, %%esp": : "b"((uintptr_t)current->stackptr));
      #endif
      Entry(current);
    }else {
      longjmp(current->context, 1);
    }
  }else {
    return;
  }
}

void co_wait(struct co *co) {
  if(co->status == CO_DEAD) {
    free_co(co);
    return;
  }
  co->waiter = current;
  current->status = CO_WAITING;
  co_yield();
}

// main is also a coroutine
void __attribute__((constructor)) before_main() {
  list = (struct co*)malloc(sizeof(struct co));
  list->name   = "main";
  list->next   = list;
  list->waiter = NULL;
  list->status = CO_RUNNING;
  list->stackptr = (void *)((((uintptr_t)list->stack+sizeof(list->stack))>>4)<<4);
  memset(list->stack, 0, sizeof(list->stack));
  current = list;
}

