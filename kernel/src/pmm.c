#include <common.h>
#include <spinlock.h>
#include <mm.h>

void *big_alloc_head;
static struct spinlock global_lock[MAX_CPU];
static struct spinlock big_alloc_lock;
static struct listhead* head[MAX_CPU];
void *tail;



//cache的最小单位为2B，但是pow2只给大内存分配(> 4096)，即最小的ret都是 1<<13 所以问题不大
static inline size_t pow2 (size_t size) {
  size_t ret = (1 << 13);
  while (size > ret) ret <<=1;
  return ret;
}

static inline void * alloc_mem (size_t size, int cpu) {
    void *ret = NULL;
    if(head[cpu] != NULL) {
      acquire(&global_lock[cpu]);
      ret = (void *)head[cpu];
      head[cpu] = (struct listhead *)head[cpu]->next;
      release(&global_lock[cpu]);
    } else {
      for (int i = 0; i < cpu_count(); ++i) {
        if(i == cpu) continue;
        if(head[i] != NULL) {
          acquire(&global_lock[i]);
          ret = (void *)head[i];
          head[i] = (struct listhead *)head[i]->next;
          release(&global_lock[i]);
          break;
        }
      }
    }
    return ret;
}

static void *kalloc(size_t size) {
  int cpu = cpu_current();

  if(size > PAGE_SIZE) {
    // TODO!!
    // 写freelist来分配
    // if (big_alloc_tot >= 3) return NULL;
    // big_alloc_tot ++;
    size_t bsize = pow2(size);
    // int page_num = bsize / PAGE_SIZE;
    // acquire(&global_lock);
    // release(&global_lock);
    void *tmp = tail;
    acquire(&big_alloc_lock);
    tail -= size; 
    tail = (void*)(((size_t)tail / bsize) * bsize);
    void *ret = tail; 
    if((uintptr_t)tail < (uintptr_t)big_alloc_head) {
      tail = tmp;
      release(&big_alloc_lock);
      return NULL;
    }
    release(&big_alloc_lock);
    return ret;
  }

  // cache的最小单位为 2B
  int item_id = 1;
  while(size > (1 << item_id)) item_id++;
  page_t *now;
  if(cache_chain[cpu][item_id] == NULL){
    cache_chain[cpu][item_id] = (page_t*) alloc_mem(PAGE_SIZE, cpu);
    Log("alloc memory addr is %p", (void *)cache_chain[cpu][item_id]);
    if(cache_chain[cpu][item_id] == NULL) return NULL; // 分配不成功
    new_page(cache_chain[cpu][item_id], cpu, item_id);
    Log("after new_page start_ptr is %p", cache_chain[cpu][item_id]->start_ptr);
  } else{
    if(full_page(cache_chain[cpu][item_id])) {
      print(FONT_RED, "the cache_chain is full, needed to allocate new space");
      //如果表头都满了，代表没有空闲的page了，分配一个page，并插在表头
      page_t* sb = (page_t*) alloc_mem(PAGE_SIZE, cpu);
      Log("alloc memory addr is %p", (void *)sb);
      if(sb == NULL) return NULL;
      new_page(sb, cpu, item_id);
      //这里注意，本来不应该用insert的，因为它是一个new的page，本身不在链表上
      insert_page_to_head(sb);
    }
  }
  now = cache_chain[cpu][item_id];
  Log("now is %p", (uintptr_t)now);
  if(now == NULL) return NULL;
  //成功找到page
  // TODO
  //应该有空位
  if(full_page(now)) assert(0);
  print(FONT_RED, "get lock!");
  
  // if(cpu_count() == 4)
  acquire(&now->lock);
  uintptr_t now_ptr = now->start_ptr + now->offset;
  void *ret = (void *)now_ptr;
  Log("now start_ptr is %p", now->start_ptr);
  Log("now offset is %p", now->offset);
  Log("now ptr is %p", now_ptr);
  //分配完，更新最新的offset，并更新它的下一个offset
  struct obj_head *objhead = (struct obj_head *)now_ptr;
  
  
  now->offset = objhead->next_offset;
  Log("use this page, the offset is %p %d", now->offset, now->offset);
  now->obj_cnt ++;
  // if(cpu_count() == 4)
  release(&now->lock);
  
  Log("Ready to judge if now is full");
  if(full_page(cache_chain[cpu][item_id])) { //已经满了
    Log("%p:cache_chain[%d][%d] is full, now->offset is %d",(void*)cache_chain[cpu][item_id], cpu, item_id, cache_chain[cpu][item_id]->offset);
    cache_chain[cpu][item_id] = cache_chain[cpu][item_id]->next;
    Log("%p:Now cache_chain is not full and now->offset is %d",(void*)cache_chain[cpu][item_id],cache_chain[cpu][item_id]->offset);
  }

  print(FONT_RED, "release lock!");
  return ret;
}


static void kfree(void *ptr) {
  return;
  // if(cpu_count() != 4) return;
  if((uintptr_t)ptr >= (uintptr_t)big_alloc_head) return; //大内存不释放
  uintptr_t page_head = ROUNDDOWN(ptr, PAGE_SIZE);
  Log("pagehead is %p", page_head);
  page_t* sb = (page_t *)page_head;
  struct obj_head* objhead = (struct obj_head*) ptr;
  if(sb->obj_cnt == 1) {
    acquire(&global_lock[sb->cpu]);
    struct listhead *empty = (struct listhead*)sb;
    if(head != NULL) {
      void *tmp = head[sb->cpu]->next;
      head[sb->cpu]->next = empty;
      empty->next = tmp;
    } else {
      head[sb->cpu] = empty;
      head[sb->cpu]->next = NULL;
    }
    release(&global_lock[sb->cpu]);
    return;
  }
  acquire(&sb->lock);
  sb->obj_cnt--;
  Log("objcnt is %d", sb->obj_cnt);
  objhead->next_offset = sb->offset;
  Log("next_offset is %d", objhead->next_offset);
  sb->offset = (uintptr_t)ptr - (uintptr_t)sb->start_ptr;
  Log("sb->offset is %d", sb->offset);
  release(&sb->lock);
  insert_page_to_head(sb);
}

static void pmm_init() {
  page_init();
  int cpu_nr = cpu_count();
  for(int i = 0; i < cpu_nr; ++i)
    initlock(&global_lock[i], "globallock");
  initlock(&big_alloc_lock, "big_alloc_lock");
  head[0] = (struct listhead*)heap.start;
  tail = heap.end;
  big_alloc_head = (void*)((uintptr_t)tail - (1 << 24)); // 给大内存分 4 MiB
  int page_nr = ((uintptr_t)big_alloc_head - (uintptr_t)head[0]) / PAGE_SIZE;
  int page_per_cpu = page_nr / cpu_nr;
  int count = 0;
  int cpu_idx = 0;
  struct listhead *walk = head[0];
  while(cpu_idx < cpu_nr && (uintptr_t)((void *)walk + PAGE_SIZE) < (uintptr_t)big_alloc_head) {
    walk->cpu = cpu_idx;
    if(count > page_per_cpu) {
      count = 0;
      walk->next = NULL;
      head[++cpu_idx] = (void *)((void *)walk + PAGE_SIZE);
    } else {
      ++count;
      walk->next = (void *)((void *)walk + PAGE_SIZE);
    }
    walk = (struct listhead *)((void *)walk + PAGE_SIZE);
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