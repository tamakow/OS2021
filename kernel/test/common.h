#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <../../abstract-machine/am/src/x86/x86.h>
#include <../../abstract-machine/klib/include/klib.h>
#include <../../abstract-machine/klib/include/klib-macros.h>
#include <stddef.h>


#define MODULE(mod) \
  typedef struct mod_##mod##_t mod_##mod##_t; \
  extern mod_##mod##_t *mod; \
  struct mod_##mod##_t

#define MODULE_DEF(mod) \
  extern mod_##mod##_t __##mod##_obj; \
  mod_##mod##_t *mod = &__##mod##_obj; \
  mod_##mod##_t __##mod##_obj

MODULE(os) {
    void (*init)();
    void (*run)();
};

MODULE(pmm) {
    void  (*init)();
    void *(*alloc)(size_t size);
    void  (*free)(void *ptr);
};

#define MiB       *(1 << 20)
#define HEAP_SIZE 128  MiB

#endif
