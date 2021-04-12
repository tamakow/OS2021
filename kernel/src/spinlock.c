#include <common.h>
#include <spinlock.h>

static int ncli[MAX_CPU] = {}; // Depth of pushcli nesting.
static int intena[MAX_CPU] = {}; // Were interrupts enabled before pushcli?


void initlock(struct spinlock *lk, char* name) {
    lk->cpu = 0;
    lk->locked = 0;
    lk->name = name;
}

void acquire(struct spinlock *lk) {
    pushcli();
}

void pushcli() {
    int eflags = get_efl();
    iset(0); //cli();
    if(ncli[cpu_current()] == 0)
        intena[cpu_current()] = eflags & FL_IF;
    intena[cpu_current()] += 1;
}

void popcli() {
    if(ienabled()) panic("popcli - interruptible");
    if(--ncli[cpu_current()] < 0) panic("popcli");
    if(ncli[cpu_current()] == 0 && intena[cpu_current()])
        iset(1);
}
