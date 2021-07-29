#include <common.h>
#include <spinlock.h>
#include <kmt.h>
#include <sem.h>
#include <limits.h>

// #define Current tasks[cpu_current()].current
// #define Head    tasks[cpu_current()].head
// #define Tail    tasks[cpu_current()].tail
// #define Idle    tasks[cpu_current()].idle

spinlock_t task_lock;
task_t task_head;
static uint32_t id_cnt = 1;
static task_t idle[MAX_CPU];

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    task->name = name;
    task->id = id_cnt++;
    task->state = RUNNABLE;
    task->context = kcontext((Area){(void*)((uintptr_t)task->stack),(void*)((uintptr_t)task->stack+STACK_SIZE)}, entry, arg);
    task->next = NULL;
    acquire(&task_lock);
    task_t *walk = &task_head;
    while(walk->next) walk = walk->next;
    walk->next = task;
    task->prev = walk;
    release(&task_lock);
    return task->id;
}

void kmt_teardown(task_t *task){
    acquire(&task_lock);
    task->state = DEADED;
    pmm->free(task->stack);
    release(&task_lock);
}

Context* kmt_schedule(Event ev, Context *context) {
    return NULL;
}

Context* kmt_context_save(Event ev, Context *context) {
    return NULL;
}

void kmt_init() {
    initlock(&task_lock, "task_lock");
    printf("helloc\n");
    task_head.id = id_cnt++;
    task_head.name = "task_head";
    task_head.next = NULL;
    task_head.prev = NULL;
    task_head.stack = pmm->alloc(STACK_SIZE);
    task_head.state = HEAD;
    task_head.context = NULL;
    
    int cpu_nr = cpu_count();
    for (int i = 0; i < cpu_nr; ++i) {
        idle[i].name = "idle";
        idle[i].next = NULL;
        idle[i].prev = NULL;
        idle[i].stack = pmm->alloc(STACK_SIZE);
        idle[i].state = RUNNABLE;
        idle[i].context = NULL;
        idle[i].id = id_cnt++;
    }
    
    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);
}

MODULE_DEF(kmt) = {
  .init=kmt_init,
  .create=kmt_create,
  .teardown=kmt_teardown, 
  .spin_init=initlock,
  .spin_lock=acquire,
  .spin_unlock=release,
  .sem_init=sem_init,
  .sem_wait=sem_wait,
  .sem_signal=sem_signal,
};


