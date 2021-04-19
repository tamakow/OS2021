#ifndef __SLAB_H__
#define __SLAB_H__

#define  KiB            *(1 << 10)
#define  PAGE_SIZE      (4 KiB)
#define  NR_SLAB_PAGE   2
#define  SLAB_SIZE      (NR_SLAB_PAGE * PAGE_SIZE)
#define  BITMAP_SIZE    28
#define  NR_ITEM_SIZE   12
#define  NR_INIT_CACHE  10

//item size 设置为{8, 16,... ,2^12} 共10项，故bitmap的最大size应该设置为不小于 7KiB / 8 = 896B ，这里为了方便，设置为 32 * BITMAP_SIZE(28)
//每个slab可以配一把自己的锁
//可以把next和prev换成freelist 和 fulllist
struct slab {
    uint8_t data[SLAB_SIZE - 1 KiB]; // 分配空间，这里给了7kiB
    int cpu;                 // 所属的cpu
    struct spinlock lock;    // 每个的锁
    int item_size;           // slab 的每个item的大小
    int item_id;             // 2^item_id = item_size
    int max_item_nr;         // 最多可以有的 item数量
    int now_item_nr;        // 现在有的item数量
    uint32_t bitmap[BITMAP_SIZE];
    struct slab *next;
    struct slab *prev;
};

struct slab* cache_chain[MAX_CPU + 1][NR_ITEM_SIZE + 1];

void slab_init();
void new_slab(struct slab *, int, int);
bool full_slab(struct slab* );
void insert_slab_to_head (struct slab* );



#endif