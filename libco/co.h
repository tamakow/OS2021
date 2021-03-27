struct co* RandomChooseCo ();
void free_co(struct co* co) ;
void entry();
struct co* co_start(const char *name, void (*func)(void *), void *arg);
void co_yield();
void co_wait(struct co *co);
