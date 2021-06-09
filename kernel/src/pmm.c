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
    void *ret;
    if((uintptr_t)head + PAGE_SIZE  > (uintptr_t)tail) ret = NULL;
    else {
       ret = head;
       head += PAGE_SIZE;
    }
    release(&global_lock);
    return ret;
}

static void *kalloc(size_t size) {
  //大内存分配 (多个cpu并行进行大内存分配，每个cpu给定固定区域, 失败)
  int cpu = cpu_current();

  if(size > PAGE_SIZE) {
    // TODO!!
    // 写freelist来分配
    size_t bsize = pow2(size);
    void *tmp = tail;
    acquire(&global_lock);
    tail -= size; 
    tail = (void*)(((size_t)tail / bsize) * bsize);
    release(&global_lock);
    void *ret = tail; 
    if((uintptr_t)tail < (uintptr_t)head) {
      tail = tmp;
      return NULL;
    }
    return ret;
  }

  // cache的最小单位为 2B
  int item_id = 1;
  while(size > (1 << item_id)) item_id++;
  slab *now;
  if(cache_chain[cpu][item_id] == NULL){
    cache_chain[cpu][item_id] = (slab*) alloc_mem(PAGE_SIZE, cpu);
    Log("alloc memory addr is %p", (void *)cache_chain[cpu][item_id]);
    if(cache_chain[cpu][item_id] == NULL) return NULL; // 分配不成功
    new_slab(cache_chain[cpu][item_id], cpu, item_id);
    Log("now1 start_ptr is %p", cache_chain[cpu][item_id]->start_ptr);
  } else{
    if(full_slab(cache_chain[cpu][item_id])) {
      print(FONT_RED, "the cache_chain is full, needed to allocate new space");
      //如果表头都满了，代表没有空闲的slab了，分配一个slab，并插在表头
      slab* sb = (slab*) alloc_mem(PAGE_SIZE, cpu);
      Log("alloc memory addr is %p", (void *)sb);
      if(sb == NULL) return NULL;
      new_slab(sb, cpu, item_id);
      //这里注意，本来不应该用insert的，因为它是一个new的slab，本身不在链表上
      insert_slab_to_head(sb);
    }
  }
  now = cache_chain[cpu][item_id];
  if(now == NULL) return NULL;
  //成功找到slab
  // TODO
  //应该有空位

  acquire(&now->lock);
  uintptr_t now_ptr = now->start_ptr + now->offset;
  void *ret = (void *)now_ptr;
  Log("now start_ptr is %p", now->start_ptr);
  Log("now offset is %p", now->offset);
  Log("now ptr is %p", now_ptr);
  //分配完，更新最新的offset，并更新它的下一个offset
  // struct obj_head *objhead = (struct obj_head *)now_ptr;
  
  //在不free的时候可以这么用
  Log("Havn't use this slab, the offset is %p %d", now->offset, now->offset);
  // objhead->next_offset = now->offset + (1 << now->obj_order);
  now->offset = now->offset + (1 << now->obj_order);
  
  Log("Ready to judge if now is full");
  if(full_slab(cache_chain[cpu][item_id])) { //已经满了
    Log("%p:cache_chain[%d][%d] is full, now->offset is %d",(void*)cache_chain[cpu][item_id], cpu, item_id, cache_chain[cpu][item_id]->offset);
    cache_chain[cpu][item_id] = cache_chain[cpu][item_id]->next;
    Log("%p:Now cache_chain is not full and now->offset is %d",(void*)cache_chain[cpu][item_id],cache_chain[cpu][item_id]->offset);
  }

  release(&now->lock);
  return ret;
}

//只是回收了slab中的对象，如果slab整个空了无法回收
static void kfree(void *ptr) {
  return ;
  // if((uintptr_t)ptr >= (uintptr_t)tail) return; //大内存不释放
  // uintptr_t slab_head = ((uintptr_t) ptr / SLAB_SIZE) * SLAB_SIZE;
  // struct slab* sb = (struct slab *)slab_head;
  // uint64_t block = ((uintptr_t)ptr - slab_head) / sb->item_size;
  // uint64_t row = block / 64, col = block % 64;
  // Log("the free ptr's cpu is %d, item_id is %d, cache_chain now is %p", sb->cpu, sb->item_id, (void*)cache_chain[sb->cpu][sb->item_id]);
  // panic_on((sb->bitmap[row] | (1ULL << col)) != sb->bitmap[row], "invalid free!!");
  // acquire(&sb->lock);
  // sb->bitmap[row] ^= (1ULL << col);
  // sb->now_item_nr--;
  // release(&sb->lock);
  // insert_slab_to_head(sb);
  // Log("Now cache_chain is %p",(void*)cache_chain[sb->cpu][sb->item_id]);
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