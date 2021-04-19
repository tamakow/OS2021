#include <common.h>
#include <slab.h>



void slab_init() {
    for(int i = 0; i < MAX_CPU + 1; ++i) {
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
    sb->now_item_nr = 0;
    memset(sb->bitmap, 0, sizeof(sb->bitmap));
    initlock(&sb->lock,"lock");
    // init circular list
    sb->next = sb;
    sb->prev = sb;
}

//判断该slab是否已满，满了返回true，否则返回false
bool full_slab(struct slab* sb) {
    assert(sb != NULL);
    return sb->now_item_nr >= sb->max_item_nr;
    // int block = -1;
    // for (int i = 0; i < 64; ++i) {
    //     if (sb->bitmap[i] != UINT64_MAX) {
    //         uint64_t tmp = 1;
    //         for(int j = 0; j < 64; ++j) {
    //             if(sb->bitmap[i] & tmp){ 
    //                 tmp <<= 1;
    //                 continue;
    //             }
    //             block = i * 64 + j;
    //             break;
    //         }
    //         break;
    //     }
    // }
    // if (block == -1) return true; // 没有必要，因为一定会有不合理的位置空出
    // if (block < sb->max_item_nr - 1) return false;
    // return true;
}


// head 即为 cache_chain[cpu][item_id]，保证head不为NULL
void insert_slab_to_head (struct slab* sb) {
    int cpu = sb->cpu;
    int id  = sb->item_id;
    // acquire(&list_lock[cpu][id]);
    //如果在链表的话，先从链表中删除
    sb->prev->next = sb->next;
    sb->next->prev = sb->prev;
    //再加在表头之前
    struct slab* listhead = cache_chain[cpu][id];
    if(listhead == NULL){
      print(FONT_RED, "Why your cache_chain[%d][%d] is NULL!!!!!!(insert_slab_to_head)", cpu, id);
      // release(&list_lock[cpu][id]);
      assert(0);
    }
    sb->next = listhead;
    sb->prev = listhead->prev;
    listhead->prev->next = sb;
    listhead->prev = sb;
    //然后把表头向前移动一位
    cache_chain[cpu][id] = sb;
    // release(&list_lock[cpu][id]);
}