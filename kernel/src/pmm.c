#include <common.h>
#include <spinlock.h>
#include <slab.h>

/*=====================Variable Definition======================*/
static int                  nr_pages;
static void*                page_start;
static bool*                flag_start;
static struct kmem_cache *  cache_start;
static struct spinlock      globallock;

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

static inline void free_pages(void* st, int num) {
  int page_index = (st - page_start) / PAGE_SIZE;
  for(int i = page_index; i < page_index + num; ++i) {
    *(flag_start + i) = false;
  }
}

/*======================Allocation Function======================*/
void Init_Kmem_Cache (struct kmem_cache * cache, size_t size){
  initlock(&cache->lock, "cachelock");
  cache->slab_item_size = size;
  cache->slabs_free = NULL;
  cache->slabs_full = NULL;

  //这个分配可能很有问题，主要是对齐可能会导致空间的浪费，从而使得slab_max_item_nr达不到，  粗暴的解决方法： 在判断的时候判断合理时直接判断 now_item_nr < max_item_nr - 1

  if(cpu_count() == 4 && size == PAGE_SIZE * 2) {
    cache->slab_alloc_pages = (size * 16 + sizeof(struct slab) - 1) / PAGE_SIZE + 1; 
    cache->slab_max_item_nr = 16;
    return;
  }

  if (size <= PAGE_SIZE / 16) {
    // 大部分小于 128 KiB
    cache->slab_alloc_pages = 2;
    cache->slab_max_item_nr = (PAGE_SIZE * 2 - sizeof(struct slab)) / size; 
  } else if (size > (1 << 20)) {
    // 大内存分配1个就够了,减1的目的是确保之后的加1不会出错
    cache->slab_alloc_pages = (size * 4 + sizeof(struct slab) - 1) / PAGE_SIZE + 1; 
    cache->slab_max_item_nr = 4;
  } else {
    cache->slab_alloc_pages = (size * 8  + sizeof(struct slab) - 1) / PAGE_SIZE + 1; 
    cache->slab_max_item_nr = 8;
  }
}

void Init_Slab(struct kmem_cache *cache, struct slab *sb) {
  sb->alloc_pages = cache->slab_alloc_pages;
  sb->cache = cache;
  sb->item_size = cache->slab_item_size;
  sb->items = NULL;
  sb->max_item_nr = cache->slab_max_item_nr;
  sb->next = NULL;
  sb->now_item_nr = 0;
  //找第一个对齐点，+sizeof(struct item)的原因是要在第一个对齐点之前插一个item
  sb->st = (void *)((((intptr_t)sb + sizeof(struct slab) + sizeof(struct item) - 1) / sb->item_size + 1) * sb->item_size);
  //sb->st -= sizeof(struct item);
}

void Init_Item(struct slab *sb, struct item *it) {
    it->used = false;
    it->slab = sb;

    //将item插入sb->items里
    //Insert_Item_In_Slab
    it->next = NULL;
    if(sb->items == NULL) sb->items = it;
    else {
      //也可以直接插在链表头
      // acquire(&sb->cache->lock);
      // it->next = sb->items;
      // sb->items = it;
      // release(&sb->cache->lock);
      struct item *walk = sb->items;
      while(walk->next) walk = walk->next;
      walk->next = it;
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

/*
 *                                        slab 的结构
 * |=========================Allocated Pages(size is cache->slab_alloc_pages)=======================|
 * |====struct slab=====|==+  ...   +==|==struct item==|== item memory==|==struct item==| .....
 * |   slab的header     | 一些无用的内存  |           第一个对齐点    
 * 出事了， 对齐不了了       
 * 将struct item 一开始就加在size里   
 */

bool New_Slab (struct kmem_cache* cache) {

  void * freehead = alloc_pages(cache->slab_alloc_pages);

  if(freehead == NULL) {
    Log("No enough space to allocate a new slab of %d pages", cache->slab_alloc_pages);

    return false;
  }
  //slab header 直接放在slab的头部
  struct slab *sb = (struct slab*) freehead;
  Init_Slab(cache, sb);

   // 初始化所有的item并将其插入到sb->items中
  struct item *it = (struct item*)(sb->st - sizeof(struct item));
  //暴力解决冲突问题(少分配一个对象)
  for(int i = 0; i < sb->max_item_nr - 1; ++i) {
    Init_Item(sb, it);
    //更新it为下一个item
    it = (struct item *) ((void*)it + sb->item_size);
  }

  //将sb放入cache->slabs_free中
  if(cache->slabs_free == NULL) cache->slabs_free = sb;
  else {
    //也可以直接插在链表头
    // sb->next = cache->slabs_free;
    // cache->slabs_free = sb;  
    struct slab* walk = cache->slabs_free;
    while(walk->next) walk = walk->next;
    walk->next = sb;
  }

  return true;
}


static void *kalloc(size_t size) {
  if(size > (1 << 24)) return NULL;
  if(cpu_count() > 3)
  acquire(&globallock);
  size = pow2(size + sizeof(struct item)); //如果size刚好是2的幂，那略浪费

  //找到size相同的cache，如果没有则申请一个
  struct kmem_cache * cache = Find_Kmem_Cache(size);


  if(cache == NULL) {
    Log("Fail to allocate a new kmem_cache");
    if(cpu_count() > 3)
    release(&globallock);
    return NULL;
  }

  //找到cache中没有full的一个slab，没有则申请一个新的slab插入到slabs_free
  if(__builtin_expect(!!(cache->slabs_free == NULL), 1)) {
    bool flag = New_Slab(cache);
    if(!flag) {
      Log("Fail to allocate a new slab");
      if(cpu_count() > 3)
      release(&globallock);
      return NULL;
    }
  }
  
  struct slab* sb = cache->slabs_free;
  struct item* it = sb->items;
  while(__builtin_expect(!!(it->used), 1)) it = it->next;

  it->used = true;
  sb->now_item_nr ++;

  if(sb->now_item_nr >= sb->max_item_nr - 1) {
    //将 sb 移动到 slabs->full

    //先从slabs_free中删除
    if(sb == cache->slabs_free) cache->slabs_free = sb->next;
    else {
      struct slab *walk = cache->slabs_free;
      while(walk && walk->next) {
        if(walk->next == sb) {
          walk->next = sb->next;
          break;
        }
        walk = walk->next;
      }
    }

    //再移动到slabs_full
    sb->next = NULL;
    if(cache->slabs_full == NULL) cache->slabs_full = sb;
    else {
      sb->next = cache->slabs_full;
      cache->slabs_full = sb;
      // struct slab* walk = cache->slabs_full;
      // while(walk->next) walk = walk->next;
      // walk->next = sb;
    }
  }
  if(cpu_count() > 3)
  release(&globallock);
  return (void *)it + sizeof(struct item);
}


static void kfree(void *ptr) {
  if(cpu_count() > 3) {
  acquire(&globallock);

  struct item* it = (struct item*)(ptr - sizeof(struct item));
  struct slab* sb = it->slab;
  struct kmem_cache* cache = sb->cache;


  it->used = false;
  sb->now_item_nr --;
  
  if(sb->now_item_nr + 1 >= sb->max_item_nr - 1) {
    //将sb从full移动到free
    if(sb == cache->slabs_full) cache->slabs_full = sb->next;
    else {
      struct slab *walk = cache->slabs_full;
      while(walk && walk->next) {
        if(walk->next == sb) {
          walk->next = sb->next;
          break;
        }
        walk = walk->next;
      }
    }

    //再移动到slabs_free
    sb->next = NULL;
    if(cache->slabs_free == NULL) cache->slabs_free = sb;
    else {
      sb->next = cache->slabs_free;
      cache->slabs_free = sb;
      // struct slab* walk = cache->slabs_free;
      // while(walk->next) walk = walk->next;
      // walk->next = sb;
    }
  }
  release(&globallock);
  }
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
  memset(cache_start, 0, NR_PAGE_CACHE * PAGE_SIZE);

  initlock(&globallock, "globallock");
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