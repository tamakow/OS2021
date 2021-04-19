#include <common.h>
#include <slab.h>

/*=====================variable definition======================*/
int                  nr_pages;
void*                page_start;
bool*                flag_start;
struct kmem_cache *  cache_start;


//根据heap_start 和 heap_end初始化
void slab_init(void *heap_start, void *heap_end) {
    intptr_t pmm_size = ((uintptr_t)heap.end - (uintptr_t)heap.start);
    
    nr_pages    = pmm_size / (PAGE_SIZE + FLAG_SIZE) - NR_PAGE_CACHE;
    page_start  = heap_start;
    flag_start  = (bool*)(page_start + (nr_pages + NR_PAGE_CACHE) * PAGE_SIZE);
    cache_start = (struct kmem_cache *) (page_start + PAGE_SIZE * nr_pages);
}