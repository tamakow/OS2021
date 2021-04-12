#include <common.h>
#include <spinlock.h>

static int ncli[MAX_CPU] = {}; // Depth of pushcli nesting.
static int intena[MAX_CPU] = {}; // Were interrupts enabled before pushcli?


void initlock(struct spinlock *lk, char* name) {
    lk->cpu = -1;
    lk->locked = 0;
    lk->name = name;
}

void acquire(struct spinlock *lk) {
    pushcli(); //avoid deadlock
    if(holding(lk)) panic("acquire");

    while(atomic_xchg((int *)&lk->locked, 1) != 0) ;
    __sync_synchronize();
    lk->cpu = cpu_current();
}

void release(struct spinlock* lk) {
    if(!holding(lk)) panic("release");
    lk->cpu = -1;
    __sync_synchronize();
    asm volatile("movl $0, %0" : "+m" (lk->locked) : );
    
    popcli();
}

bool holding(struct spinlock* lk) {
    bool r = false;
    pushcli();
    r = lk->locked && lk->cpu == cpu_current();
    popcli();
    return r;
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
        iset(1); // sti();
}
