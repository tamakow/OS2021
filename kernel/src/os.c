#include <common.h>
#include <os.h>
#include <devices.h>

static struct os_irq irq_head = {
  0, EVENT_NULL, NULL, NULL
};

static inline task_t *task_alloc() {
  return pmm->alloc(sizeof(task_t));
}

static void tty_reader(void *arg) {
  device_t *tty = dev->lookup(arg);
  char cmd[128], resp[128], ps[16];
  snprintf(ps, 16, "(%s) $ ", arg);
  while (1) {
    tty->ops->write(tty, 0, ps, strlen(ps));
    int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
    cmd[nread] = '\0';
    sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
    tty->ops->write(tty, 0, resp, strlen(resp));
  }
}

static void os_init() {
  pmm->init();
  KLog("pmm init ok!");
  kmt->init();
  KLog("kmt init ok!");
  dev->init();
   kmt->create(task_alloc(), "tty_reader", tty_reader, "tty1");
  kmt->create(task_alloc(), "tty_reader", tty_reader, "tty2");
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
  KLog("add irq with seq number %d", seq);
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
