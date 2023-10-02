#ifndef _K_PROCESS_H
#define _K_PROCESS_H
#include <type.h>
#include "pagetable.h"
#include "trap.h"
#include "machine.h"
#include "syscall.h"
#include "file.h"
#include "fs_file.h"

#define NUM_PROCS 256
#define NUM_FILES 64
#define USER_STACK_SIZE 0x100000
#define KERNEL_STACK_SIZE 0x100000
#define MAX_CPUS 64
#define HEAP_TOP 0x4000000000

#define FLAGS_WAITED 0x1

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

    struct pcb_t *parent;
    int flags;
    int exit_code;

    // page table info
    pfn_t ptbase_pfn;
    vpn_t ptbase_vpn;
    PTReference_2 *ptref_base;
    void *brk;
    void *heap_bottom;

    // trap frame
    TrapContext *trapframe;

    // kernel stack, pointing to guard page.
    pfn_t kernel_stack_pfn;
    vpn_t kernel_stack_vpn;

    // filesystem: CWD, opened files
    FSFile cwd_file;
    FileDescriptor opened_files[NUM_FILES];
} PCB;


typedef struct {
    PCB *running;
} CPU;
extern PCB process_control_table[NUM_PROCS];
extern CPU cpus[MAX_CPUS];



i64 load_process(PCB *process, File *program);
void unload_process(PCB *process);
void free_process_space(PCB *process);
void init_scheduler(void);
void schedule(int cpuid);

static inline __attribute__((always_inline)) void terminate(PCB *process) {
    process->status = TERMINATED;
}
#endif