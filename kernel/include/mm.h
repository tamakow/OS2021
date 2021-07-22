#ifndef __MM_H__
#define __MM_H__

#define  KiB            *(1 << 10)
#define  PAGE_SIZE      (32 KiB)
#define  HDR_SIZE       (1 << 7)
#define  NR_ITEM_SIZE   12

//item size 设置为{2, 4, 8, 16,... ,4096} 共12项
typedef union page {
    struct {
        struct spinlock lock; //锁，用于串行化分配和并发的free
        int obj_cnt; //页面已分配的对象数，减少到0的时候回收页面
        int max_obj; //最多可分配的对象数
        uintptr_t offset; //start_ptr + offset就是目前分配到的地方，在这个位置上有一个obj_head代表下一个可供分配的位置
        union page *next; //同一个cpu下的其他页面
        int obj_order; //分配的大小为2^obj_order
        uintptr_t start_ptr; //第一个数据的起始位置
        int cpu;
    };
    struct {
        uint8_t header[HDR_SIZE];
        uint8_t data[PAGE_SIZE - HDR_SIZE];
  } __attribute__((packed));
} page_t;

struct obj_head {
    uint16_t next_offset; //下一个分配的地址离start_ptr的offset
};

struct listhead {
    void *next;
    int cpu;
};

typedef struct cache {
    page_t *available_list;
    page_t *full_list;
}cache_t;


cache_t* cache_chain[MAX_CPU + 1][NR_ITEM_SIZE + 1];

void page_init();
void new_page(page_t *, int, int);
bool full_page(page_t *);
void move_page_to_full(page_t *, cache_t *);
void move_page_to_available(page_t *, cache_t *);
#endif