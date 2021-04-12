#include <common.h>
#include <spinlock.h>

//first just try one lock and kalloc
static void *head;
static size_t tmp = 0;
struct spinlock lk;
static void *kalloc(size_t size) {
  acquire(&lk);
  if(tmp > 0)
    head = head + tmp;
  if((uintptr_t)head > (uintptr_t)heap.end) return NULL;
  tmp = 0;
  size_t i = 1;
  while(i < size) i<<=1;
  printf("page is %d\n",(1<<i));
  size_t j = ((size_t)head / i + 1)* i;
  printf("j = %d\n",j);
  while((uintptr_t)head < j) head++;
  tmp = size;
  printf("head is %p\n",head);
  release(&lk);
  return head;
}

static void kfree(void *ptr) {
}

static void pmm_init() {
  head = heap.start;
  initlock(&lk,"lalala");
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
