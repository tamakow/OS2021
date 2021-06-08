#include <common.h>
#include <spinlock.h>
#include <slab.h>


static struct spinlock global_lock;
void *tail, *head;
//cache的最小单位为8B，但是pow2只给大内存分配，所以问题不大
static inline size_t pow2 (size_t size) {
  size_t ret = 1;
  while (size > ret) ret <<=1;
  return ret;
}

static inline void * alloc_mem (size_t size, int cpu) {
  acquire(&global_lock);
  return NULL;
}

static void *kalloc(size_t size) {
  //大内存分配 (多个cpu并行进行大内存分配，每个cpu给定固定区域, 失败)
  int cpu = cpu_current();

  if(size > PAGE_SIZE) {
    // 写freelist来分配
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

static void pmm_init() {
  slab_init();
  initlock(&global_lock, "global_lock");
  head = heap.start;
  tail = heap.end;
  Log("%d",cpu_count());
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};