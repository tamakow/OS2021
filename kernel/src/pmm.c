#include <common.h>
#include <spinlock.h>
#include <slab.h>


//第二次尝试，先只用slab试试，大内存分配先不管（）,好，大内存分配直接炸了
static struct spinlock global_lock;
static struct spinlock big_alloc_lock;
// static struct spinlock list_lock[MAX_CPU + 1][NR_ITEM_SIZE + 1];
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

// head 即为 cache_chain[cpu][item_id]，保证head不为NULL
void insert_slab_to_head (struct slab* sb) {
    int cpu = sb->cpu;
    int id  = 1;
    while(id <= NR_ITEM_SIZE && (1 << id) != sb->item_size) id++;
    assert((1 << id) == sb->item_size);
    // acquire(&list_lock[cpu][id]);
    //如果在链表的话，先从链表中删除
    sb->prev->next = sb->next;
    sb->next->prev = sb->prev;
    //再加在表头之前
    struct slab* listhead = cache_chain[cpu][id];
    if(listhead == NULL){
      print(FONT_RED, "Why your cache_chain[%d][%d] is NULL!!!!!!(insert_slab_to_head)", cpu, id);
      // release(&list_lock[cpu][id]);
      assert(0);
    }
    sb->next = listhead;
    sb->prev = listhead->prev;
    listhead->prev->next = sb;
    listhead->prev = sb;
    //然后把表头向前移动一位
    cache_chain[cpu][id] = sb;
    // release(&list_lock[cpu][id]);
}


static void *kalloc(size_t size) {
  //大内存分配
  if(size > PAGE_SIZE) {
    size_t bsize = 1;
    while(bsize < size) bsize <<= 1;
    acquire(&big_alloc_lock);
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
    // init circular list
    now->next = now;
    now->prev = now;
  } else{
    if(full_slab(cache_chain[cpu][item_id])) {
      //如果表头都满了，代表没有空闲的slab了，分配一个slab，并插在表头
      struct slab* sb = (struct slab*) alloc_mem(SLAB_SIZE);
      if(sb == NULL) return NULL;
      new_slab(sb, cpu, item_size);
      sb->next = sb;
      sb->prev = sb;
      //这里注意，本来不应该用insert的，因为它是一个new的slab，本身不在链表上
      insert_slab_to_head(sb);
    }
    now = cache_chain[cpu][item_id];
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
        release(&now->lock);
        block = i * 64 + j;
        break;
      }
      break;
    }
  }
  if(block >= now->max_item_nr - 1) { //已经满了
    cache_chain[cpu][item_id] = cache_chain[cpu][item_id]->next; 
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
  insert_slab_to_head(sb);
}

#ifndef TEST
static void pmm_init() {
  initlock(&global_lock,"GlobalLock");
  initlock(&big_alloc_lock,"big_lock");
  // for(int i = 0; i <= MAX_CPU; ++i) {
  //   for(int j = 0; j <= NR_ITEM_SIZE; ++j) {
  //       initlock(&list_lock[i][j], "listlock");
  //   }
  // }
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
