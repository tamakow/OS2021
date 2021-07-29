#ifndef __OS_H__
#define __OS_H__

#include <common.h>

struct os_irq {
    int seq, event;
    handler_t handler;
    struct os_irq *next;
};

#endif