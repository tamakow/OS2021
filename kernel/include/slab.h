#ifndef __SLAB_H__
#define __SLAB_H__

#define  KiB            *(1 << 10)
#define  PAGE_SIZE      (32 KiB)
#define  HDR_SIZE       (1 << 7)
#define  NR_ITEM_SIZE   12
#define  OFFSET_MAX     UINT64_MAX
//item size 设置为{2, 4, 8, 16,... ,2^12} 共12项，故bitmap的最大size应该设置为不小于 7KiB / 2 = 3.5KiB
//每个slab可以配一把自己的锁
//可以把next和prev换成freelist 和 fulllist
// struct slab {
//     uint8_t data[SLAB_SIZE - 1 KiB]; // 分配空间，这里给了7kiB
//     int cpu;                 // 所属的cpu
//     struct spinlock lock;    // 每个的锁
//     int item_size;           // slab 的每个item的大小
//     int item_id;             // 2^item_id = item_size
//     int max_item_nr;         // 最多可以有的 item数量
//     int now_item_nr;        // 现在有的item数量
//     uint64_t bitmap[BITMAP_SIZE];
//     struct slab *next;
//     struct slab *prev;
// };


typedef union slab {
    struct {
        struct spinlock lock; //锁，用于串行化分配和并发的free
        int obj_cnt; //页面已分配的对象数，减少到0的时候回收页面
        uintptr_t offset; //start_ptr + offset就是目前分配到的地方，在这个位置上有一个obj_head代表下一个可供分配的位置
        union slab *next; //同一个cpu下的其他页面
        union slab *prev;
        int obj_order; //分配的大小为2^obj_order
        uintptr_t start_ptr; //第一个数据的起始位置
    };
    struct {
        uint8_t header[HDR_SIZE];
        uint8_t data[PAGE_SIZE - HDR_SIZE];
  } __attribute__((packed));
} slab;

struct obj_head {
    uint16_t next_offset; //下一个分配的地址离start_ptr的offset
};


slab* cache_chain[MAX_CPU + 1][NR_ITEM_SIZE + 1];

void slab_init();
void new_slab(slab *, int, int);
bool full_slab(slab* );
void insert_slab_to_head (slab* );

#endif