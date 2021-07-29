#include <common.h>
#include <os.h>

static struct os_irq irq_head = {
  0, EVENT_NULL, NULL, NULL
};


static void os_init() {
  pmm->init();
}

static void os_run() {
  iset(true);
  while (1);
  panic("os run should not touch here!");
}

static Context *os_trap(Event ev, Context *context) {
  Context *ret = NULL;
  for(struct os_irq *h = irq_head.next; h != NULL; h = h->next) {
     if (h->event == EVENT_NULL || h->event == ev.event) {
      Context *r = h->handler(ev, context);
      panic_on(r && ret, "returning multiple contexts");
      if (r) ret = r;
    }
  }
  panic_on(!ret, "returning NULL context");
  return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
  struct os_irq *walk = &irq_head;
  struct os_irq *new_irq = pmm->alloc(sizeof(struct os_irq));
  new_irq->seq = seq;
  new_irq->event = event;
  new_irq->handler = handler;
  new_irq->next = NULL;
  while (walk->next && walk->next->seq < seq) 
    walk = walk->next;
  new_irq->next = walk->next;
  walk->next = new_irq;
  printf("add irq, seq is %d\n", seq);
}



MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq =os_on_irq,
};
