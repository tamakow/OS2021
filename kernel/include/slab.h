#ifndef __SLAB_H__
#define __SLAB_H__

#define  KiB            *(1 << 10)
#define  PAGE_SIZE      4 KiB

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

struct kmem_cache {
    struct list_head slabs_partial;
    struct list_head slabs_full;
    struct list_head slabs_free;
};

#endif