#include <common.h>
#include <spinlock.h>


void initlock (struct spinlock *lk, char* name) {
    lk->cpu = 0;
    lk->locked = 0;
    lk->name = name;
}

