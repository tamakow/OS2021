#include <common.h>
#include <spinlock.h>
#include <slab.h>

/*=====================Variable Definition======================*/
static int                  nr_pages;
static void*                page_start;
static bool*                flag_start;
static struct kmem_cache *  cache_start;

/*=====================Helpers Function=========================*/
static inline size_t pow2 (size_t size) {
  size_t ret = 1;
  while (size > ret) ret <<=1;
  return ret;
}

static inline void* alloc_pages (int num) {
  for(int i = 0; i + num < nr_pages;) {
    int j = 0;
    for(j = 0; j < num; ++j){
      if(*(flag_start + i + j)){
        break;
      }
    }
    if(j == num) {
      //找到符合要求的, 直接返回
      for(int k = 0; k < num; ++k){
        *(flag_start + i + k) = true;
      }
      return page_start + i * PAGE_SIZE;
    } else {
      i += (j + 1);
    }
  }
  return NULL;
}


/*======================Allocation Function======================*/
void Init_Kmem_Cache (struct kmem_cache * cache, size_t size){
  cache->cpu = cpu_current();
  initlock(&cache->lock, "cachelock");
  cache->slab_item_size = size;
  cache->slabs_free = NULL;
  cache->slabs_full = NULL;
  cache->slabs_partial = NULL;

  //这个分配可能很有问题，主要是对齐可能会导致空间的浪费，从而使得slab_max_item_nr达不到，  粗暴的解决方法： 在判断的时候判断合理时直接判断 now_item_nr < max_item_nr - 1
  if (size <= PAGE_SIZE / 16) {
    // 大部分小于 256 KiB
    cache->slab_alloc_pages = 1;
    cache->slab_max_item_nr = (PAGE_SIZE - sizeof(struct slab)) / (size + sizeof(struct item)); 
  } else {
    // 大内存分配四个就够了
    cache->slab_alloc_pages = ((size + sizeof(struct item)) * 4 + sizeof(struct slab)) / PAGE_SIZE + 1; 
    cache->slab_max_item_nr = 4;
  }
}


struct kmem_cache* Find_Kmem_Cache(size_t size) {
  struct kmem_cache *walk = cache_start;
  while((intptr_t)walk < (intptr_t)flag_start && walk->slab_item_size && walk->slab_item_size != size) walk++; //保证所有没有使用的cache上的item_size不大于0
  if((intptr_t)walk >= (intptr_t)flag_start) {
    Log("No enough space to allocate a new kmem_cache of size %d", size);
    return NULL;
  }
  if(walk->slab_item_size == size) return walk;
  // 申请一个大小为size的新cache 
  Init_Kmem_Cache(walk, size); 
  return walk;
}

void New_Slab (struct kmem_cache* cache) {
  void * freehead = alloc_pages(cache->slab_alloc_pages);
  if(freehead == NULL) {
    Log("No enough space to allocate a new slab of %d pages", cache->slab_alloc_pages);
    return;
  }
  //TODO() 2
}


static void *kalloc(size_t size) {
  size = pow2(size);

  //找到size相同的cache，如果没有则申请一个
  struct kmem_cache * cache = Find_Kmem_Cache(size);
  if(cache == NULL) {
    Log("Fail to allocate a new kmem_cache");
    return NULL;
  }

  //找到cache中没有full的一个slab，没有则申请一个新的slab插入到slabs_free
  if(cache->slabs_partial == NULL && cache->slabs_free == NULL) {
    New_Slab(cache);
  }
  if(cache->slabs_partial == NULL && cache->slabs_free == NULL) {
    Log("Fail to allocate a new slab")
    return NULL;
  }
  //TODO() 3
  
  return NULL;
}


static void kfree(void *ptr) {
  return;
}

#ifndef TEST
static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  
  /*=============================slab_init=====================================*/
  nr_pages    = pmsize / (PAGE_SIZE + FLAG_SIZE) - NR_PAGE_CACHE;
  page_start  = heap.start;
  flag_start  = (bool*)(page_start + (nr_pages + NR_PAGE_CACHE) * PAGE_SIZE);
  cache_start = (struct kmem_cache *) (page_start + PAGE_SIZE * nr_pages);

  memset(flag_start, 0, nr_pages * FLAG_SIZE);
}
#else
static void pmm_init() {
  initlock(&global_lock,"GlobalLock");
  initlock(&lk,"lk");
  initlock(&big_alloc_lock,"big_lock");
  slab_init();

  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  head = heap.start;
  tail = heap.end;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
}
#endif

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};