#ifndef __KMT_H__
#define __KMT_H__

#define STACK_SIZE 4096
#define Current current[cpu_current()]


enum task_states {
    BLOCKED = 1, RUNNABLE, DEADED, HEAD,
};

struct task {
    struct {
      const char *name;
      int state;
      int id;
      struct task* next;
      struct task* prev;
      Context *context;  
    };
    uint8_t* stack;  
};

// struct task_list {
//     task_t *current;
//     task_t *head;
//     task_t *tail;
//     task_t idle;
// };

// typedef struct task_list tl_t; //task_list type

task_t* current[MAX_CPU];


void kmt_init();
int kmt_create(task_t *, const char *, void (*)(void *), void *);
void kmt_teardown(task_t *);
Context* kmt_schedule(Event, Context *);
Context* kmt_context_save(Event, Context *);
#endif