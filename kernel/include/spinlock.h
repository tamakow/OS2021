#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <stdbool.h>

/*
    from xv6
*/

struct spinlock {
    bool locked;
    const char *name;
    int cpu;
};

void initlock(struct spinlock *, const char*);
void acquire(struct spinlock *);
void release(struct spinlock *);
bool holding(struct spinlock *);
void pushcli();
void popcli();

#endif 