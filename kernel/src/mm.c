#include <common.h>
#include <mm.h>

int page_cnt = 0;

void page_init() {
    for(int i = 0; i < MAX_CPU + 1; ++i) {
        for(int j = 0; j < NR_ITEM_SIZE + 1; ++j) {
            cache_chain[i][j] = NULL;
        }
    }
}

void new_page(page_t * sb, int cpu, int item_id) {
    assert(sb != NULL);
    int size = (1 << item_id);
    char name[128];
    sprintf(name, "lock%d",page_cnt++);
    Log("%s", name);
    initlock(&sb->lock, name);
    sb->obj_cnt = 0;
    sb->obj_order = item_id;
    sb->start_ptr = (uintptr_t)(((uintptr_t)(sb->data) - 1) / size + 1) * size; 
    sb->offset = 0;
    sb->max_obj = (int)((uintptr_t)sb + PAGE_SIZE - sb->start_ptr) / size;
    sb->cpu = cpu;
    Log("sb's max_obj is %d", sb->max_obj);
    
    // init circular list
    sb->next = sb;
    sb->prev = sb;
    // init obj_head;
    // 只在未分配的obj上有用，已分配的无所谓
    uintptr_t offset = 0;
    while(sb->start_ptr + offset < (uintptr_t)sb + PAGE_SIZE) {
        struct obj_head* objhead = (struct obj_head*) (sb->start_ptr + offset);
        objhead->next_offset = offset + size;
        offset += size;
        if(sb->start_ptr + offset > (uintptr_t)sb + PAGE_SIZE)
            break;
    }
#ifdef DEBUG
    struct obj_head* objhead = (struct obj_head*) sb->start_ptr;
#endif
    Log("offset is %p", offset);
    Log("objhead->next_offset is %p", objhead->next_offset);
    Log("start_ptr is %p",sb->start_ptr);
    Log("sb is %p",(uintptr_t)sb);
    Log("end ptr is %p", (uintptr_t)sb + PAGE_SIZE);
}

//判断该page是否已满，满了返回true，否则返回false
bool full_page(page_t* sb) {
    assert(sb != NULL);
    //这里判断下一个分配的位置到底是否有所需分配的那么大的
    return sb->obj_cnt >= sb->max_obj;
}


// head 即为 cache_chain[cpu][item_id]，保证head不为NULL
void insert_page_to_head (page_t* sb) {
    int cpu = sb->cpu;
    int order  = sb->obj_order;
    //如果在链表的话，先从链表中删除
    sb->prev->next = sb->next;
    sb->next->prev = sb->prev;
    //再加在表头之前
    page_t* head = cache_chain[cpu][order];
    // if(cpu_count() == 4) {
    while(!full_page(head) && head != sb) {
        head = head->next;
    }
    // }
    if(head == NULL){
      print(FONT_RED, "Why your cache_chain[%d][%d] is NULL!!!!!!(insert_page_to_head)", cpu, order);
      assert(0);
    }
    if(head == sb) return;
    sb->next = head;
    sb->prev = head->prev;
    head->prev->next = sb;
    head->prev = sb;
    
    
    //然后把表头向前移动一位
    if(head == cache_chain[cpu][order])
        cache_chain[cpu][order] = sb;
}