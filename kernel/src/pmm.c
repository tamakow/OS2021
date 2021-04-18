#include <common.h>
#include <spinlock.h>
#include <slab.h>


//第二次尝试，先只用slab试试，大内存分配先不管（）
static struct spinlock global_lock;
static struct spinlock lk; 
void *head;



static void *kalloc(size_t size) {
  if(size > PAGE_SIZE) return NULL; //拒绝大内存分配
  int cpu = cpu_current();
  int item_id = 1;
  while(size > (1 << item_id)) item_id++;
  struct slab *now;
  if(cache_chain[cpu][item_id] == NULL){
    acquire(&global_lock);
    if(head + SLAB_SIZE  > heap.end){
       release(&global_lock);
       return NULL;
    }
    cache_chain[cpu][item_id] = (struct slab*) head;
    head += SLAB_SIZE;
    release(&global_lock);
    new_slab(cache_chain[cpu][item_id], cpu, (1<<item_id));
    now = cache_chain[cpu][item_id];
  } else{
    now = cache_chain[cpu][item_id];
    struct slab *walk = NULL;
    while(now && full_slab(now)) {
      walk = now;
      now = now->next;
    }
    if(now == NULL) {
      acquire(&global_lock);
      if(head + SLAB_SIZE  > heap.end){
        release(&global_lock);
        return NULL;
      }
      now = (struct slab*) head;
      head += SLAB_SIZE;
      release(&global_lock);
      new_slab(now, cpu, (1<<item_id));
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
        acquire(&lk);
        now->bitmap[i] |= (1ULL<<j);
        block = i*64 + j;
        release(&lk);
        break;
      }
      break;
    }
  }
  void *ret = (void*) ((uintptr_t)((uintptr_t)now + block * (1<<item_id)));
  return ret;
}

//只是回收了slab中的对象，如果slab整个空了无法回收
static void kfree(void *ptr) {
  uintptr_t slab_head = ((uintptr_t) ptr / SLAB_SIZE) *SLAB_SIZE;
  struct slab* sb = (struct slab *)slab_head;
  uint64_t block = ((uintptr_t)ptr - slab_head) / sb->item_size;
  uint64_t row = block / 64, col = block % 64;
  acquire(&lk);
  sb->bitmap[row] ^= (1ULL << col);
  release(&lk);
}

static void pmm_init() {
  initlock(&global_lock,"GlobalLock");
  initlock(&lk,"lk");
  slab_init();
  head = heap.start;
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
