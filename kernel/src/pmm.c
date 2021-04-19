#include <common.h>
#include <spinlock.h>
#include <slab.h>


static struct spinlock global_lock;
static struct spinlock big_alloc_lock;
void *head;
void *tail;

static inline size_t pow2 (size_t size) {
  size_t ret = 1;
  while (size > ret) ret <<=1;
  return ret;
}

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
    size_t bsize = pow2(size);
    void *tmp = tail;
    acquire(&big_alloc_lock);
    tail -= size; 
    tail = (void*)(((size_t)tail / bsize) * bsize);
    void *ret = tail; 
    release(&big_alloc_lock);
    if((uintptr_t)tail < (uintptr_t)head) {
      tail = tmp;
      return NULL;
    }
    return ret;
  }

  int cpu = cpu_current();
  int item_id = 1;
  while(size > (1 << item_id)) item_id++;
  struct slab *now;
  if(cache_chain[cpu][item_id] == NULL){
    cache_chain[cpu][item_id] = (struct slab*) alloc_mem(SLAB_SIZE);
    if(cache_chain[cpu][item_id] == NULL) return NULL; // 分配不成功
    new_slab(cache_chain[cpu][item_id], cpu, item_id);
  } else{
    if(full_slab(cache_chain[cpu][item_id])) {
      print(FONT_RED, "the cache_chain is full, needed to allocate new space");
      //如果表头都满了，代表没有空闲的slab了，分配一个slab，并插在表头
      struct slab* sb = (struct slab*) alloc_mem(SLAB_SIZE);
      if(sb == NULL) return NULL;
      new_slab(sb, cpu, item_id);
      //这里注意，本来不应该用insert的，因为它是一个new的slab，本身不在链表上
      insert_slab_to_head(sb);
    }
  }
  now = cache_chain[cpu][item_id];
  if(now == NULL) return NULL;
  //成功找到slab
  uint64_t block = 0;
  for(int i = 0; i < 64; ++i) {
    if(now->bitmap[i] != UINT64_MAX) {
      uint64_t tmp = 1;
      for(int j = 0; j < 64; ++j) {
        if(now->bitmap[i] & tmp){ 
          tmp <<= 1;
          continue;
        }
        acquire(&now->lock);
        now->bitmap[i] |= tmp;
        now->now_item_nr++;
        release(&now->lock);
        block = i * 64 + j;
        break;
      }
      break;
    }
  }
    Log("Ready to judge if now is full");
  if(full_slab(cache_chain[cpu][item_id])) { //已经满了
    Log("%p:cache_chain[%d][%d] is full, now->now_item_nr is %d",(void*)cache_chain[cpu][item_id], cpu, item_id,cache_chain[cpu][item_id]->now_item_nr);
    cache_chain[cpu][item_id] = cache_chain[cpu][item_id]->next;
    Log("%p:Now cache_chain is not full and now->now_item_nr is %d",(void*)cache_chain[cpu][item_id],cache_chain[cpu][item_id]->now_item_nr);
  }
  return (void*) ((uintptr_t)((uintptr_t)now + block * now->item_size));
}

//只是回收了slab中的对象，如果slab整个空了无法回收
//暂时只回收PAGE_SIZE
static void kfree(void *ptr) {
  return;
  if((uintptr_t)ptr >= (uintptr_t)tail) return; //大内存不释放
  uintptr_t slab_head = ((uintptr_t) ptr / SLAB_SIZE) * SLAB_SIZE;
  struct slab* sb = (struct slab *)slab_head;
  //tmp
  if(sb->item_id != 12) return;
  uint64_t block = ((uintptr_t)ptr - slab_head) / sb->item_size;
  uint64_t row = block / 64, col = block % 64;
  Log("the free ptr's cpu is %d, item_id is %d, cache_chain now is %p", sb->cpu, sb->item_id, (void*)cache_chain[sb->cpu][sb->item_id]);
  panic_on((sb->bitmap[row] | (1ULL << col)) != sb->bitmap[row], "free wrong!!");
  acquire(&sb->lock);
  sb->bitmap[row] ^= (1ULL << col);
  sb->now_item_nr--;
  release(&sb->lock);
  insert_slab_to_head(sb);
  Log("Now cache_chain is %p with now_item_nr is %d",(void*)cache_chain[sb->cpu][sb->item_id], sb->now_item_nr);
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
  //给每个链表先分个十个slab再说
  for(int i = 0; i < cpu_count() + 1; ++i) {
    for(int j = 0; j < NR_ITEM_SIZE + 1; ++j) {
        cache_chain[i][j] = (struct slab*) alloc_mem(SLAB_SIZE);
        if(cache_chain[i][j] == NULL) return; // 分配不成功,直接退出初始化
        new_slab(cache_chain[i][j], i, j);
        int num = 1;    
        while(num <= NR_INIT_CACHE){
          num++;
          struct slab* now = (struct slab*) alloc_mem(SLAB_SIZE);
          if(now == NULL) return;
          new_slab(now,i,j);
          insert_slab_to_head(now); 
        }
    }
  }
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
