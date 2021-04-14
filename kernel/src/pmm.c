#include <common.h>
#include <spinlock.h>

//first just try one lock and kalloc
static void *head;
struct spinlock lk;
static void *kalloc(size_t size) {
  acquire(&lk);
  if((uintptr_t)head > (uintptr_t)heap.end){ 
    release(&lk);
    return NULL;
  }
  size_t i = 1;
  while(i < size) i<<=1;
  // while((size_t)head % i != 0) head++;
  head = (void *)(((size_t)head / i + 1) * i) + size; 
  release(&lk);
  return head - size;
}

static void kfree(void *ptr) {

}

static void pmm_init() {
  head = heap.start;
  initlock(&lk,"GlobalLock");
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
