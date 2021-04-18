#include <common.h>
#include <spinlock.h>
#include <slab.h>


//第二次尝试，先只用slab试试，大内存分配先不管（）,好，大内存分配直接炸了
static struct spinlock global_lock;
static struct spinlock big_alloc_lock;
void *head;
void *tail;

static inline void * alloc_mem (size_t size) {
    acquire(&global_lock);
    void *ret;
    if((uintptr_t)head + SLAB_SIZE  > (uintptr_t)tail) ret = NULL;
    else {
       ret = head;
       head += SLAB_SIZE;
    }
    release(&global_lock);
    return ret;
}

static void *kalloc(size_t size) {
  //大内存分配
  if(size > PAGE_SIZE) {
    acquire(&big_alloc_lock);
    size_t bsize = 1;
    while(bsize < size) bsize <<= 1;
    tail -= bsize; 
    tail = (void*)(((size_t)tail / bsize) * bsize);
    void *ret = tail; 
    release(&big_alloc_lock);
    if((uintptr_t)tail < (uintptr_t)head) {
      return NULL;
    }
    return ret;
  }

  int cpu = cpu_current();
  int item_id = 1;
  while(size > (1 << item_id)) item_id++;
  int item_size = 1 << item_id;
  struct slab *now;
  if(cache_chain[cpu][item_id] == NULL){
    cache_chain[cpu][item_id] = (struct slab*) alloc_mem(SLAB_SIZE);
    if(cache_chain[cpu][item_id] == NULL) return NULL; // 分配不成功
    new_slab(cache_chain[cpu][item_id], cpu, item_size);
    now = cache_chain[cpu][item_id];
  } else{
    now = cache_chain[cpu][item_id];
    struct slab *walk = NULL;
    // TOO SLOW!!!
    while(now && full_slab(now)) {
      walk = now;
      now = now->next;
    }
    if(now == NULL) {
      now = (struct slab*) alloc_mem(SLAB_SIZE);
      if(now == NULL) return NULL;
      new_slab(now, cpu, item_size);
      walk->next = now;
    }
  }
  if(now == NULL) return NULL;
  //成功找到slab
  uint64_t block = 0;
  for(int i = 0; i < 64; ++i) {
    if(now->bitmap[i] != UINT64_MAX) {
      for(int j = 0; j < 64; ++j) {
        if(now->bitmap[i] & (1ULL << j)) continue;
        acquire(&now->lock);
        now->bitmap[i] |= (1ULL<<j);
        block = i*64 + j;
        release(&now->lock);
        break;
      }
      break;
    }
  }
  return (void*) ((uintptr_t)((uintptr_t)now + block * item_size));
}

//只是回收了slab中的对象，如果slab整个空了无法回收
static void kfree(void *ptr) {
  if((uintptr_t)ptr >= (uintptr_t)tail) return; //大内存不释放
  uintptr_t slab_head = ((uintptr_t) ptr / SLAB_SIZE) * SLAB_SIZE;
  struct slab* sb = (struct slab *)slab_head;
  uint64_t block = ((uintptr_t)ptr - slab_head) / sb->item_size;
  uint64_t row = block / 64, col = block % 64;
  acquire(&sb->lock);
  sb->bitmap[row] ^= (1ULL << col);
  release(&sb->lock);
}

#ifndef TEST
static void pmm_init() {
  initlock(&global_lock,"GlobalLock");
  initlock(&big_alloc_lock,"big_lock");
  slab_init();
  head = heap.start;
  tail = heap.end;
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
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
