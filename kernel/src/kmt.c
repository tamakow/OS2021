#include <common.h>
#include <spinlock.h>
#include <kmt.h>
#include <sem.h>
#include <limits.h>

// #define Idle    idle[cpu_current()]
// #define Current current[cpu_current()]

spinlock_t task_lock;
task_t task_head;
static uint32_t id_cnt = 0;
static task_t idle[MAX_CPU];
task_t* current[MAX_CPU];
task_t* last[MAX_CPU];

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    task->name = name;
    task->state = RUNNABLE;
    task->stack = pmm->alloc(STACK_SIZE);
    task->context = kcontext((Area){(void*)((uintptr_t)task->stack),(void*)((uintptr_t)task->stack+STACK_SIZE)}, entry, arg);
    task->next = NULL;
    acquire(&task_lock);
    task->id = id_cnt++;
    task_t *walk = &task_head;
    while(walk->next) walk = walk->next;
    walk->next = task;
    release(&task_lock);
    return task->id;
}

void kmt_teardown(task_t *task){
    acquire(&task_lock);
    panic_on(task->state == RUNNING, "teardown false!");
    task->state = DEADED;
    pmm->free(Current->stack);
    release(&task_lock);
}

Context* kmt_schedule(Event ev, Context *context) {
    if(Current->state == DEADED) {
        task_t* walk = &task_head;
        while(walk && walk->next != Current) walk = walk->next;
        panic_on(!walk, "can not find this task");
        walk->next = Current->next;
    }
    acquire(&task_lock);
    task_t *ret = NULL;
    for (task_t *walk = &task_head; walk != NULL; walk = walk->next) {
        if(walk == Current) continue;
        if(walk->state == RUNNABLE) {
            ret = walk;
            break;
        }
    }
    if(!ret) ret = &Idle;
    ret->state = RUNNING;
    panic_on(Last, "last current has not done!");
    if(Current != &Idle)
        Last = Current;
    Current = ret;
    release(&task_lock);
    return ret->context;
}

Context* kmt_context_save(Event ev, Context *context) {
    if(Last) {
        Last->state = RUNNABLE;
        Last = NULL;
    }
    Current->context = context;
    return NULL;
}

void ientry() {
    iset(true);
    while(1) yield();
}

void kmt_init() {
    initlock(&task_lock, "task_lock");
    task_head.id = id_cnt++;
    task_head.name = "task_head";
    task_head.next = NULL;
    task_head.stack = pmm->alloc(STACK_SIZE);
    task_head.state = HEAD;
    task_head.context = NULL;

    int cpu_nr = cpu_count();
    for (int i = 0; i < cpu_nr; ++i) {
        idle[i].name = "idle";
        idle[i].next = NULL;
        idle[i].stack = pmm->alloc(STACK_SIZE);
        idle[i].state = RUNNABLE;
        idle[i].context = kcontext((Area){(void*)((uintptr_t)idle[i].stack),(void*)((uintptr_t)idle[i].stack+STACK_SIZE)}, ientry, NULL);
        idle[i].id = -1;

        current[i] = &idle[i];
    }
    
    os->on_irq(INT32_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT32_MAX, EVENT_NULL, kmt_schedule);
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


