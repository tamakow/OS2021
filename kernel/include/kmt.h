#ifndef __KMT_H__
#define __KMT_H__

#define STACK_SIZE 4096
#define Current current[cpu_current()]
#define Idle    idle[cpu_current()]

enum task_states {
    BLOCKED = 1, RUNNABLE, DEADED, HEAD, RUNNING,
};

struct task {
    struct {
      const char *name;
      int state;
      int id;
      int time;
      int cpu;
      struct task* next;
      Context *context;  
    };
    uint8_t* stack;  
};


void kmt_init();
int kmt_create(task_t *, const char *, void (*)(void *), void *);
void kmt_teardown(task_t *);
Context* kmt_schedule(Event, Context *);
Context* kmt_context_save(Event, Context *);
#endif