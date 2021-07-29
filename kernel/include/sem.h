#ifndef __SEM_H__
#define __SEM_H__


struct semaphore {
  struct spinlock lock;
  const char *name;
  int value;
};

void sem_init(sem_t *, const char *, int);
void sem_wait(sem_t *);
void sem_signal(sem_t *);

#endif