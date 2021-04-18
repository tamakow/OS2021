#ifndef __SLAB_H__
#define __SLAB_H__

#define  KiB            *(1 << 10)
#define  PAGE_SIZE      (4 KiB)
#define  NR_SLAB_PAGE   2
#define  SLAB_SIZE      (NR_SLAB_PAGE * PAGE_SIZE)
#define  BITMAP_SIZE    64
#define  NR_ITEM_SIZE   12
// #define  UINT64_MAX     ((1<<64) - 1) 
//item size 设置为{2, 4, 8, 16,... ,2^12} 共十二项，故bitmap的最大size应该设置为不小于 SLAB_SIZE / 2 = 3.5 KiB ，这里为了方便，设置为 4 KiB = 64 * BITMAP_SIZE

struct slab {
    uint8_t data[SLAB_SIZE - 1 KiB]; // 分配空间，这里给了7kiB
    int cpu;                 // 所属的cpu
    int item_size;           // slab 的每个item的大小
    int max_item_nr;         // 最多可以有的 item数量
    uint64_t bitmap[BITMAP_SIZE];
    struct slab *next;
};


#endif