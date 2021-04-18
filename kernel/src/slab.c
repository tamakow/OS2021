#include <common.h>
#include <slab.h>

void slab_init() {
    for(int i = 0; i < MAX_CPU + 1; ++ i) {
        for(int j = 0; j < NR_ITEM_SIZE + 1; ++j) {
            cache_chain[i][j] = NULL;
        }
    }
}

void new_slab(struct slab * sb, int cpu, int item_id) {
    assert(sb != NULL);
    sb->cpu = cpu;
    sb->item_id = item_id;
    sb->item_size = (1 << item_id);
    sb->max_item_nr = (SLAB_SIZE - 1 KiB) / sb->item_size;
    memset(sb->bitmap, 0, sizeof(sb->bitmap));
    initlock(&sb->lock,"lock");
    sb->next = NULL;
    sb->prev = NULL;
}

//判断该slab是否已满，满了返回true，否则返回false
bool full_slab(struct slab* sb) {
    assert(sb != NULL);
    int block = -1;
    for (int i = 0; i < 64; ++i) {
        if (sb->bitmap[i] != UINT64_MAX) {
            block = i * 64;
            int cnt = 0;
            while (cnt < 64 && (sb->bitmap[i] & (1ULL << cnt))) cnt++;
            block += cnt;
            break;
        }
    }
    if (block == -1) return true; // 没有必要，因为一定会有不合理的位置空出
    if (block < sb->max_item_nr - 1) return false;
    return true;
}


