#include <common.h>
#include <spinlock.h>
#include <slab.h>


struct big_page {
  void *start_ptr; //为了对齐可能会不是从page的head开始分配起
  int  alloc_slab; //分配的
};

static struct big_page big_alloc[10];
static int big_alloc_tot = 0;
static struct spinlock global_lock;
static struct spinlock big_alloc_lock;
void *tail;
void *big_alloc_head;
static struct freelist* head;

static inline void big_alloc_init() {
  for (int i = 0; i < 10; ++i) {
    big_alloc[i].start_ptr = NULL;
    big_alloc[i].alloc_slab = 0;
  }
}


//cache的最小单位为2B，但是pow2只给大内存分配(> 4096)，即最小的ret都是 1<<13 所以问题不大
static inline size_t pow2 (size_t size) {
  size_t ret = (1 << 13);
  while (size > ret) ret <<=1;
  return ret;
}

static inline void * alloc_mem (size_t size) {
    void *ret = (void *)head;
    if(head->next != NULL) {
      acquire(&global_lock);
      head = head->next;
      release(&global_lock);
    }
    return ret;
}

int cnt = 0;
static void *kalloc(size_t size) {
  int cpu = cpu_current();

  if(size > PAGE_SIZE) {
    // TODO!!
    // 写freelist来分配
    if (big_alloc_tot >= 3) return NULL;
    big_alloc_tot ++;
    size_t bsize = pow2(size);
    // int slab_num = bsize / PAGE_SIZE;
    // acquire(&global_lock);
    // release(&global_lock);
    void *tmp = tail;
    acquire(&big_alloc_lock);
    tail -= size; 
    tail = (void*)(((size_t)tail / bsize) * bsize);
    release(&big_alloc_lock);
    void *ret = tail; 
    if((uintptr_t)tail < (uintptr_t)big_alloc_head) {
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
    cache_chain[cpu][item_id] = (slab*) alloc_mem(PAGE_SIZE);
    Log("alloc memory addr is %p", (void *)cache_chain[cpu][item_id]);
    if(cache_chain[cpu][item_id] == NULL) return NULL; // 分配不成功
    new_slab(cache_chain[cpu][item_id], cpu, item_id);
    Log("after new_slab start_ptr is %p", cache_chain[cpu][item_id]->start_ptr);
  } else{
    if(full_slab(cache_chain[cpu][item_id])) {
      print(FONT_RED, "the cache_chain is full, needed to allocate new space");
      //如果表头都满了，代表没有空闲的slab了，分配一个slab，并插在表头
      slab* sb = (slab*) alloc_mem(PAGE_SIZE);
      Log("alloc memory addr is %p", (void *)sb);
      if(sb == NULL) return NULL;
      new_slab(sb, cpu, item_id);
      //这里注意，本来不应该用insert的，因为它是一个new的slab，本身不在链表上
      insert_slab_to_head(sb);
    }
  }
  now = cache_chain[cpu][item_id];
  Log("now is %p", (uintptr_t)now);
  if(now == NULL) return NULL;
  //成功找到slab
  // TODO
  //应该有空位
  if(full_slab(now)) assert(0);
  print(FONT_RED, "get lock!");
  
  if(cpu_count() == 4)
  acquire(&now->lock);
  uintptr_t now_ptr = now->start_ptr + now->offset;
  void *ret = (void *)now_ptr;
  Log("now start_ptr is %p", now->start_ptr);
  Log("now offset is %p", now->offset);
  Log("now ptr is %p", now_ptr);
  //分配完，更新最新的offset，并更新它的下一个offset
  struct obj_head *objhead = (struct obj_head *)now_ptr;
  
  
  now->offset = objhead->next_offset;
  Log("use this slab, the offset is %p %d", now->offset, now->offset);
  now->obj_cnt ++;
  if(cpu_count() == 4)
  release(&now->lock);
  
  Log("Ready to judge if now is full");
  if(full_slab(cache_chain[cpu][item_id])) { //已经满了
    Log("%p:cache_chain[%d][%d] is full, now->offset is %d",(void*)cache_chain[cpu][item_id], cpu, item_id, cache_chain[cpu][item_id]->offset);
    cache_chain[cpu][item_id] = cache_chain[cpu][item_id]->next;
    Log("%p:Now cache_chain is not full and now->offset is %d",(void*)cache_chain[cpu][item_id],cache_chain[cpu][item_id]->offset);
  }

  print(FONT_RED, "release lock!");
  return ret;
}


//只是回收了slab中的对象，如果slab整个空了无法回收
static void kfree(void *ptr) {
  if(cpu_count() != 4) return;
  if((uintptr_t)ptr >= (uintptr_t)big_alloc_head) return; //大内存不释放
  uintptr_t slab_head = ROUNDDOWN(ptr, PAGE_SIZE);
  Log("slabhead is %p", slab_head);
  slab* sb = (slab *)slab_head;
  struct obj_head* objhead = (struct obj_head*) ptr;
  acquire(&sb->lock);
  sb->obj_cnt--;
  Log("objcnt is %d", sb->obj_cnt);
  objhead->next_offset = sb->offset;
  Log("next_offset is %d", objhead->next_offset);
  sb->offset = (uintptr_t)ptr - (uintptr_t)sb->start_ptr;
  Log("sb->offset is %d", sb->offset);
  release(&sb->lock);
  if(sb->obj_cnt == 0) {
    acquire(&global_lock);
    struct freelist *empty = (struct freelist*)sb;
    void *tmp = head->next;
    head->next = empty;
    empty->next = tmp;
    release(&global_lock);
  }
  else insert_slab_to_head(sb);
}

static void pmm_init() {
  slab_init();
  big_alloc_init();
  initlock(&global_lock, "globallock");
  initlock(&big_alloc_lock, "big_alloc_lock");
  head = (struct freelist*)heap.start;
  tail = heap.end;
  big_alloc_head = (void*)((uintptr_t)tail - 4 * (1 << 20));
  struct freelist *walk = head;
  while((uintptr_t)(walk + PAGE_SIZE) < (uintptr_t)big_alloc_head) {
    walk->next = (void *)(walk + PAGE_SIZE);
    walk = (struct freelist *)walk->next;
  }
  walk->next = NULL;
  Log("%d",cpu_count());
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};