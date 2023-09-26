#ifndef _K_TRAP_H
#define _K_TRAP_H
#include <stdint.h>

#define TRAP_InstructionMisaligned 0
#define TRAP_InstructionFault 1
#define TRAP_IllegalInstruction 2
#define TRAP_Breakpoint 3
#define TRAP_LoadFault 5
#define TRAP_StoreMisaligned 6
#define TRAP_StoreFault 7
#define TRAP_UserEnvCall 8
#define TRAP_VirtualSupervisorEnvCall 10
#define TRAP_InstructionPageFault 12
#define TRAP_LoadPageFault 13
#define TRAP_StorePageFault 15
#define TRAP_InstructionGuestPageFault 20
#define TRAP_LoadGuestPageFault 21
#define TRAP_VirtualInstruction 22
#define TRAP_StoreGuestPageFault 23
#define TRAP_USER_ENV_CALL 8
#define TRAP_STORE_FAULT 5
#define TRAP_STORE_PAGE_FAULT 11
#define TRAP_ILLEGAL_INSTRUCTION 2

typedef struct {
    uint64_t x[32];
    uint64_t sstatus;
    uint64_t sepc;
    uint64_t kernel_satp;
    uint64_t kernel_sp;
    uint64_t trap_handler;
} TrapContext;

void app_init_context(TrapContext *ptr, uint64_t entry, uint64_t user_sp, uint64_t kernel_sp);

#endif