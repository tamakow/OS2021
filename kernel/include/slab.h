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

struct slab* cache_chain[MAX_CPU + 1][NR_ITEM_SIZE + 1];

void slab_init() {
    for(int i = 0; i < MAX_CPU + 1; ++ i) {
        for(int j = 0; j < NR_ITEM_SIZE + 1; ++j) {
            cache_chain[i][j] = NULL;
        }
    }
}

void new_slab(struct slab * sb, int cpu, int item_size) {
    assert(sb != NULL);
    sb->cpu           = cpu;
    sb->item_size     = item_size;
    sb->max_item_nr   = SLAB_SIZE / sb->item_size;
    memset(sb->bitmap, 0, sizeof(sb->bitmap));
    sb->next          = NULL;
}

//判断该slab是否已满，满了返回true，否则返回false
bool full_slab(struct slab* sb) {
    assert(sb != NULL);
    int block = -1;
    for(int i = 0; i < 64; ++i) {
        if(sb->bitmap[i] != UINT64_MAX) {
            block = i * 64;
            int cnt = 0;
            while (cnt < 64 && (sb->bitmap[i] & (1ULL << cnt))) cnt++;
            block += cnt;
            break;
        }
    }
    if(block == -1) return true; // 没有必要，因为一定会有不合理的位置空出
    if(block < sb->max_item_nr) return false;
    return true;
}

#endif