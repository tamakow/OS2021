#include <common.h>
#include <spinlock.h>
#include <slab.h>


//第二次尝试，先只用slab试试，大内存分配先不管（）,好，大内存分配直接炸了
static struct spinlock global_lock;
static struct spinlock lk; 
static struct spinlock big_alloc_lock;
void *head;
void *tail;

struct slab* cache_chain[MAX_CPU + 1][NR_ITEM_SIZE + 1];

void slab_init() {
    for(int i = 0; i < MAX_CPU + 1; ++ i) {
        for(int j = 0; j < NR_ITEM_SIZE + 1; ++j) {
            cache_chain[i][j] = NULL;
        }
    }
}

void new_slab(struct slab * sb, int cpu, int item_size) {
    assert(sb != NULL);
    sb->cpu           = cpu;
    sb->item_size     = item_size;
    sb->max_item_nr   = SLAB_SIZE / sb->item_size;
    memset(sb->bitmap, 0, sizeof(sb->bitmap));
    sb->next          = NULL;
}

//判断该slab是否已满，满了返回true，否则返回false
bool full_slab(struct slab* sb) {
    assert(sb != NULL);
    int block = -1;
    for(int i = 0; i < 64; ++i) {
        if(sb->bitmap[i] != UINT64_MAX) {
            block = i * 64;
            int cnt = 0;
            while (cnt < 64 && (sb->bitmap[i] & (1ULL << cnt))) cnt++;
            block += cnt;
            break;
        }
    }
    if(block == -1) return true; // 没有必要，因为一定会有不合理的位置空出
    if(block < sb->max_item_nr) return false;
    return true;
}

static void *kalloc(size_t size) {
  //大内存分配
  if(size > PAGE_SIZE) {
    acquire(&big_alloc_lock);
    size_t bsize = 1;
    while(bsize < size) bsize <<= 1; 
    tail -= bsize; 
    tail = (void*)(((size_t)tail / bsize) * bsize);
    release(&big_alloc_lock);
    return tail;
  }

  // cache
  int cpu = cpu_current();
  int item_id = 1;
  while(size > (1 << item_id)) item_id++;
  struct slab *now;
  if(cache_chain[cpu][item_id] == NULL){
    acquire(&global_lock);
    if((uintptr_t)head + SLAB_SIZE  > (uintptr_t)tail){
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
      if((uintptr_t)head + SLAB_SIZE  > (uintptr_t)tail){
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
  if((uintptr_t)ptr >= (uintptr_t)tail) return; //大内存不释放
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
  initlock(&big_alloc_lock,"big_lock");
  slab_init();
  head = heap.start;
  tail = heap.end;
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
