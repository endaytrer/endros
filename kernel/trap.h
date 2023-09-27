#ifndef _K_TRAP_H
#define _K_TRAP_H
#include <stdint.h>

#define TRAP_Exception 0
#define TRAP_Exception_InstructionMisaligned 0
#define TRAP_Exception_InstructionFault 1
#define TRAP_Exception_IllegalInstruction 2
#define TRAP_Exception_Breakpoint 3
#define TRAP_Exception_LoadFault 5
#define TRAP_Exception_StoreMisaligned 6
#define TRAP_Exception_StoreFault 7
#define TRAP_Exception_UserEnvCall 8
#define TRAP_Exception_VirtualSupervisorEnvCall 10
#define TRAP_Exception_InstructionPageFault 12
#define TRAP_Exception_LoadPageFault 13
#define TRAP_Exception_StorePageFault 15
#define TRAP_Exception_InstructionGuestPageFault 20
#define TRAP_Exception_LoadGuestPageFault 21
#define TRAP_Exception_VirtualInstruction 22
#define TRAP_Exception_StoreGuestPageFault 23

#define TRAP_Interrupt_UserSoft 0 
#define TRAP_Interrupt_SupervisorSoft 1 
#define TRAP_Interrupt_VirtualSupervisorSoft 2 
#define TRAP_Interrupt_UserTimer 4 
#define TRAP_Interrupt_SupervisorTimer 5 
#define TRAP_Interrupt_VirtualSupervisorTimer 6 
#define TRAP_Interrupt_UserExternal 8 
#define TRAP_Interrupt_SupervisorExternal 9 
#define TRAP_Interrupt_VirtualSupervisorExternal 10

#define TRAP_Interrupt ((uint64_t)1 << 63)
#define TRAP_CODE(trap) ((trap) & ~TRAP_Interrupt)
#define SIE_STIMER (1 << 5)

typedef struct {
    uint64_t x[32];
    uint64_t sstatus;
    uint64_t sepc;
    uint64_t kernel_satp;
    uint64_t kernel_sp;
    uint64_t trap_handler;
} TrapContext;

void trap_return(int cpuid);
void app_init_context(TrapContext *ptr, uint64_t entry, uint64_t user_sp, uint64_t kernel_sp);

#endif