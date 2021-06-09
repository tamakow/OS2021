#include <common.h>
#include <slab.h>

void slab_init() {
    for(int i = 0; i < MAX_CPU + 1; ++i) {
        for(int j = 0; j < NR_ITEM_SIZE + 1; ++j) {
            cache_chain[i][j] = NULL;
        }
    }
}

void new_slab(slab * sb, int cpu, int item_id) {
    assert(sb != NULL);
    int size = (1 << item_id);
    initlock(&sb->lock,"lock");
    sb->obj_cnt = 0;
    sb->obj_order = item_id;
    //去掉减1可以partial ac？
    sb->start_ptr = (uintptr_t)(((uintptr_t)(&sb->data) - 1) / size + 1) * size; 
    sb->offset = 0;
    // init circular list
    sb->next = sb;
    sb->prev = sb;
    // init obj_head;
    // 只在未分配的obj上有用，已分配的无所谓
    struct obj_head* objhead = (struct obj_head*) sb->start_ptr;
    objhead->next_offset = size;
    Log("start_ptr is %p",sb->start_ptr);
}

//buggy
//判断该slab是否已满，满了返回true，否则返回false
bool full_slab(slab* sb) {
    assert(sb != NULL);
    //这里判断下一个分配的位置到底是否有所需分配的那么大的
    int size = (1 << sb->obj_order); 
    uintptr_t ptr = sb->start_ptr + sb->offset; 
    return ptr + size > (uintptr_t)sb + PAGE_SIZE;
}


// head 即为 cache_chain[cpu][item_id]，保证head不为NULL
void insert_slab_to_head (slab* sb) {
    int cpu = cpu_current();
    int order  = sb->obj_order;
    //如果在链表的话，先从链表中删除
    sb->prev->next = sb->next;
    sb->next->prev = sb->prev;
    //再加在表头之前
    slab* listhead = cache_chain[cpu][order];
    if(listhead == NULL){
      print(FONT_RED, "Why your cache_chain[%d][%d] is NULL!!!!!!(insert_slab_to_head)", cpu, order);
      assert(0);
    }
    sb->next = listhead;
    sb->prev = listhead->prev;
    listhead->prev->next = sb;
    listhead->prev = sb;
    //然后把表头向前移动一位
    cache_chain[cpu][order] = sb;
}