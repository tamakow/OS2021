#include <stdbool.h>

struct spinlock {
    bool locked;
    char *name;
    int cpu;
};

void initlock(struct spinlock *, char*);
void acquire(struct spinlock *);
void pushcli();
void popcli();