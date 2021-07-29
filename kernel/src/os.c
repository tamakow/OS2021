#include <common.h>
#include <os.h>
#include <devices.h>

static struct os_irq irq_head = {
  0, EVENT_NULL, NULL, NULL
};

sem_t empty, fill;
#define P kmt->sem_wait
#define V kmt->sem_signal

void producer(void *arg) { while (1) { P(&empty); putch('('); V(&fill);  } }
void consumer(void *arg) { while (1) { P(&fill);  putch(')'); V(&empty); } }

static inline task_t *task_alloc() {
  return pmm->alloc(sizeof(task_t));
}

static void os_init() {
  pmm->init();
  KLog("pmm init ok!");
  kmt->init();
  KLog("kmt init ok!");
  // dev->init();
  kmt->sem_init(&empty, "empty", 5);  // 缓冲区大小为 5
  kmt->sem_init(&fill,  "fill",  0);
  for (int i = 0; i < 4; i++) // 4 个生产者
    kmt->create(task_alloc(), "producer", producer, NULL);
  for (int i = 0; i < 5; i++) // 5 个消费者
    kmt->create(task_alloc(), "consumer", consumer, NULL);

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
}



MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq =os_on_irq,
};
