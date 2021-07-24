#include <common.h>
#include <spinlock.h>
#include <mm.h>

void *big_alloc_head;
static struct spinlock global_lock[MAX_CPU];
static struct spinlock big_alloc_lock;
static struct listhead* head[MAX_CPU];
static int cpu_nr;
static int big_alloc_tot = 0;
void *tail;


//cache的最小单位为2B，但是pow2只给大内存分配(> 4096)，即最小的ret都是 1<<13 所以问题不大
static inline size_t pow2 (size_t size) {
  size_t ret = (1 << 13);
  while (size > ret) ret <<=1;
  return ret;
}

static inline void *alloc_mem (int cpu) {
    void *ret = NULL;
    if(head[cpu] != NULL) {
      acquire(&global_lock[cpu]);
      ret = (void *)head[cpu];
      head[cpu] = (struct listhead *)head[cpu]->next;
      release(&global_lock[cpu]);
    } else {
      for (int i = 0; i < cpu_nr; ++i) {
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

  if(size > (4 KiB)) {
    // TODO!!
    // 写freelist来分配
    if (big_alloc_tot >= 3) return NULL;
    big_alloc_tot ++;
    size_t bsize = pow2(size);
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
  if(cache_chain[cpu][item_id]->available_list == NULL){
    cache_chain[cpu][item_id]->available_list = (page_t*) alloc_mem(cpu);
    Log("alloc memory addr is %p", (void *)cache_chain[cpu][item_id]->available_list);
    if(cache_chain[cpu][item_id]->available_list == NULL) return NULL; // 分配不成功
    new_page(cache_chain[cpu][item_id]->available_list, cpu, item_id);
    Log("after new_page start_ptr is %p", cache_chain[cpu][item_id]->available_list->start_ptr);
  }
  page_t* now = cache_chain[cpu][item_id]->available_list;
  Log("now is %p", (uintptr_t)now);
  if(now == NULL) return NULL;
  //成功找到page
  // TODO
  //应该有空位
  if(full_page(now)) assert(0);
  Log("after new_page lock is %d", (int)now->lock.locked);
  // if(cpu_count() == 4)
  acquire(&now->lock);
  print(FONT_RED, "get lock!");
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
  if(full_page(now)) { //已经满了
    move_page_to_full(now, cache_chain[cpu][item_id]);
  }
  
  print(FONT_RED, "release lock!");
  return ret;
}


static void kfree(void *ptr) {
  return;
  if((uintptr_t)ptr >= (uintptr_t)big_alloc_head) return; //大内存不释放
  uintptr_t page_head = ROUNDDOWN(ptr, PAGE_SIZE);
  Log("pagehead is %p", page_head);
  page_t* sb = (page_t *)page_head;
  struct obj_head* objhead = (struct obj_head*) ptr;
  if(sb->obj_cnt == 1 && sb->obj_order != 12) {
    acquire(&global_lock[sb->cpu]);
    struct listhead *empty = (struct listhead*)sb;
    if(head[sb->cpu] != NULL) {
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
  if(sb->obj_cnt >= sb->max_obj)
    move_page_to_available(sb, cache_chain[sb->cpu][sb->obj_order]);
  acquire(&sb->lock);
  sb->obj_cnt--;
  Log("objcnt is %d", sb->obj_cnt);
  objhead->next_offset = sb->offset;
  Log("next_offset is %d", objhead->next_offset);
  sb->offset = (uintptr_t)ptr - (uintptr_t)sb->start_ptr;
  Log("sb->offset is %d", sb->offset);
  release(&sb->lock);
}

static void pmm_init() {
  page_init();
  cpu_nr = cpu_count();
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