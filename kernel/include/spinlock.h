#include <stdbool.h>

struct spinlock {
    bool locked;
    char *name;
    int cpu;
};

