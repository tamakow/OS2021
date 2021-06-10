#ifndef __SLAB_H__
#define __SLAB_H__

#define  KiB            *(1 << 10)
#define  PAGE_SIZE      (32 KiB)
#define  HDR_SIZE       (1 << 7)
#define  NR_ITEM_SIZE   12

//item size 设置为{2, 4, 8, 16,... ,2^12} 共12项
typedef union slab {
    struct {
        struct spinlock lock; //锁，用于串行化分配和并发的free
        int obj_cnt; //页面已分配的对象数，减少到0的时候回收页面
        int max_obj; //最多可分配的对象数
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