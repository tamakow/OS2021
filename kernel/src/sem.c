#include <common.h>
#include <sem.h>
#include <kmt.h>

void sem_init(sem_t *sem, const char *name, int value){
    initlock(&sem->lock, "sem_lock");
    sem->name = name;
    sem->value = value;
    sem->head_idx = 0;
    sem->tail_idx = 0;
}

void sem_wait(sem_t *sem){
    bool flag = false;
    acquire(&sem->lock);
    --sem->value;
    if(sem->value < 0) {
        flag = true;
        Current->state = BLOCKED;
        sem->queue[sem->tail_idx] = Current;
        sem->tail_idx = (sem->tail_idx + 1) % QLEN;
        panic_on(sem->tail_idx == sem->head_idx, "sem_wait queue overflow!");
    }
    release(&sem->lock);
    if(flag) yield();
}
  
void sem_signal(sem_t *sem){
    acquire(&sem->lock);
    ++sem->value;
    if (sem->value <= 0){
        if(sem->head_idx != sem->tail_idx) {
            sem->queue[sem->head_idx]->state = RUNNABLE;
            sem->head_idx = (sem->head_idx + 1) % QLEN;
        }
    }
    release(&sem->lock);
}