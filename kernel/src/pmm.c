#include <common.h>
#include <spinlock.h>
#include <slab.h>


static inline size_t pow2 (size_t size) {
  size_t ret = 1;
  while (size > ret) ret <<=1;
  return ret;
}

static void *kalloc(size_t size) {
  return NULL;
}


static void kfree(void *ptr) {
  return;
}

#ifndef TEST
static void pmm_init() {
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