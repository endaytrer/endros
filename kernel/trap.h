#ifndef _K_TRAP_H
#define _K_TRAP_H
#include <stdint.h>

#define TRAP_USER_ENV_CALL 8
#define TRAP_STORE_FAULT 5
#define TRAP_STORE_PAGE_FAULT 11
#define TRAP_ILLEGAL_INSTRUCTION 2

typedef struct {
    uint64_t x[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapContext;

void app_init_context(TrapContext *ptr, void *entry, uint64_t sp);

#endif