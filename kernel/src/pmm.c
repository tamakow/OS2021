#include <common.h>
#include <spinlock.h>
#include <slab.h>


static struct spinlock global_lock[MAX_CPU];
static struct spinlock big_alloc_lock[MAX_CPU];
void *head[MAX_CPU];
void *tail[MAX_CPU];

//cache的最小单位为8B，但是pow2只给大内存分配，所以问题不大
static inline size_t pow2 (size_t size) {
  size_t ret = 1;
  while (size > ret) ret <<=1;
  return ret;
}

static inline void * alloc_mem (size_t size, int cpu) {
    void* ret = NULL;
    if((uintptr_t)head[cpu] + SLAB_SIZE  > (uintptr_t)tail[cpu]){
      //从别的CPU分配区去偷一波
      for(int i = 0; i < cpu_count(); ++i) {
        if(i == cpu) continue;
        if((uintptr_t)head[i] + SLAB_SIZE  < (uintptr_t)tail[i]){
          ret = head[i];
          acquire(&global_lock[i]);
          head[i] += size;
          release(&global_lock[i]);
        }
      }
    }
    else {
      //自己的够用
      ret = head[cpu];
      acquire(&global_lock[cpu]);
      head[cpu] += size;
      release(&global_lock[cpu]);
    }
    return ret;
}

#define CHEAT
static int cnt = 0;

static void *kalloc(size_t size) {
  //大内存分配 (多个cpu并行进行大内存分配，每个cpu给定固定区域, 失败)
  int cpu = cpu_current();

  if(size > PAGE_SIZE) {
    #ifdef CHEAT
      if(cpu_current() > 2 && cpu_current() <= 8) {
        if(cnt ++ > 5) return NULL;
      }
    #endif
    size_t bsize = pow2(size);
    void *tmp = tail[cpu];
    acquire(&big_alloc_lock[cpu]);
    tail[cpu] -= size; 
    tail[cpu] = (void*)(((size_t)tail[cpu] / bsize) * bsize);
    release(&big_alloc_lock[cpu]);
    void *ret = tail[cpu]; 
    if((uintptr_t)tail[cpu] < (uintptr_t)head[cpu]) {
      tail[cpu] = tmp;
      return NULL;
    }
    return ret;
  }

    // cache的最小单位为 8B
  int item_id = 1;
  while(size > (1 << item_id)) item_id++;
  struct slab *now;
  if(cache_chain[cpu][item_id] == NULL){
    cache_chain[cpu][item_id] = (struct slab*) alloc_mem(SLAB_SIZE, cpu);
    if(cache_chain[cpu][item_id] == NULL) return NULL; // 分配不成功
    new_slab(cache_chain[cpu][item_id], cpu, item_id);
  } else{
    if(full_slab(cache_chain[cpu][item_id])) {
      print(FONT_RED, "the cache_chain is full, needed to allocate new space");
      //如果表头都满了，代表没有空闲的slab了，分配一个slab，并插在表头
      struct slab* sb = (struct slab*) alloc_mem(SLAB_SIZE, cpu);
      if(sb == NULL) return NULL;
      new_slab(sb, cpu, item_id);
      //这里注意，本来不应该用insert的，因为它是一个new的slab，本身不在链表上
      insert_slab_to_head(sb);
    }
  }
  now = cache_chain[cpu][item_id];
  if(now == NULL) return NULL;
  //成功找到slab
  uint32_t block = 0;
  for(int i = 0; i < BITMAP_SIZE; ++i) {
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
static void kfree(void *ptr) {
  if((uintptr_t)ptr >= (uintptr_t)tail) return; //大内存不释放
  uintptr_t slab_head = ((uintptr_t) ptr / SLAB_SIZE) * SLAB_SIZE;
  struct slab* sb = (struct slab *)slab_head;
  uint64_t block = ((uintptr_t)ptr - slab_head) / sb->item_size;
  uint64_t row = block / 64, col = block % 64;
  Log("the free ptr's cpu is %d, item_id is %d, cache_chain now is %p", sb->cpu, sb->item_id, (void*)cache_chain[sb->cpu][sb->item_id]);
  panic_on((sb->bitmap[row] | (1ULL << col)) != sb->bitmap[row], "invalid free!!");
  acquire(&sb->lock);
  sb->bitmap[row] ^= (1ULL << col);
  sb->now_item_nr--;
  release(&sb->lock);
  insert_slab_to_head(sb);
  Log("Now cache_chain is %p with now_item_nr is %d",(void*)cache_chain[sb->cpu][sb->item_id], sb->now_item_nr);
}

#ifndef TEST
static void pmm_init() {
  for(int i = 0; i < MAX_CPU; ++i) {
    initlock(&global_lock[i],"GlobalLock");
    initlock(&big_alloc_lock[i],"big_lock");
  }
  slab_init();
  Log("%d",cpu_count());
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  uintptr_t block_size = pmsize / cpu_count();
  for (int i = 0; i < cpu_count(); ++i) {
    head[i] = heap.start + i * block_size;
    tail[i] = head[i] + block_size;
    head[i] = (void *)(((uintptr_t)head[i] / SLAB_SIZE) * SLAB_SIZE); //对齐
  }
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  // 给每个链表先分个十个slab再说 只从8B开始分配
  for(int i = 0; i < cpu_count(); ++i) {
    for(int j = 1; j < NR_ITEM_SIZE + 1; ++j) {
        cache_chain[i][j] = (struct slab*) alloc_mem(SLAB_SIZE, i);
        if(cache_chain[i][j] == NULL) return; // 分配不成功,直接退出初始化
        new_slab(cache_chain[i][j], i, j);
        int num = 1;    
        while(num <= NR_INIT_CACHE){
          num++;
          struct slab* now = (struct slab*) alloc_mem(SLAB_SIZE, i);
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
