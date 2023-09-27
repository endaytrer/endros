#ifndef _K_PROCESS_H
#define _K_PROCESS_H
#include <stdint.h>
#include "pagetable.h"
#include "trap.h"
#include "machine.h"
#define NUM_PROCS 256
#define USER_STACK_SIZE 0x100000
#define KERNEL_STACK_SIZE 0x100000
#define APP_SIZE_LIMIT 0x20000

typedef int pid_t;

typedef struct pcb_t {
    pid_t pid;
    enum {
        UNUSED,
        READY,
        RUNNING,
        BLOCKED,
        ZOMBIE,
        TERMINATED
    } status;

    int cpuid;
    int flags;
    // page table info
    pfn_t ptbase_pfn;
    vpn_t ptbase_vpn;
    PTReference_2 *ptref_base;

    // trap frame
    TrapContext *trapframe;

    // kernel stack, pointing to guard page.
    pfn_t kernel_stack_pfn;
    vpn_t kernel_stack_vpn;
} PCB;


typedef struct {
    PCB *running;
} CPU;
extern PCB process_control_table[NUM_PROCS];
extern CPU cpus[NCPU];



void load_process(PCB *process, pid_t pid, void *app_start, int cpuid);
void unload_process(PCB *process);
void init_scheduler(void);
void schedule(int cpuid);

static inline __attribute__((always_inline)) void terminate(PCB *process) {
    process->status = TERMINATED;
}
#endif