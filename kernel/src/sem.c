#include <common.h>
#include <sem.h>
#include <kmt.h>

extern task_t *current[MAX_CPU];

void sem_init(sem_t *sem, const char *name, int value){
    initlock(&sem->lock, "sem_lock");
    sem->name = name;
    sem->value = value;
}

void sem_wait(sem_t *sem){
    
    return;
}
  
void sem_signal(sem_t *sem){
    
    return;
}