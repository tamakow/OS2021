#include <common.h>
#include <spinlock.h>
#include <slab.h>


// just try one lock 
static void *head;
static struct spinlock lk;


static void *kalloc(size_t size) {
  acquire(&lk);

  size_t i = 1;
  while(i < size) i<<=1;
  
  head = (void *)(((size_t)head / i + 1) * i); 
  void *ret = head;
  head += size;

  release(&lk);
  if((uintptr_t)head > (uintptr_t)heap.end) return NULL;
  return ret;
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
