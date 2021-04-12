#include <common.h>
#include <spinlock.h>

//first just try one lock and kalloc
static void *head;
static size_t tmp = 0;
static void *kalloc(size_t size) {
  if(tmp > 0)
    head = head + tmp;
  if((uintptr_t)head > (uintptr_t)heap.end) return NULL;
  tmp = 0;
  int i = 0;
  while((1<<i) < size) i++;
  while((uintptr_t)head % (1<<i) != 0) head++;
  tmp = size;
  return head;
}

static void kfree(void *ptr) {
}

static void pmm_init() {
  head = heap.start;
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
