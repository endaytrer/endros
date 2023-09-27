#include "syscall.h"
#include "printk.h"
#include "machine.h"
#include "timer.h"
#include "process.h"

int64_t sys_yield(void) {
    int cpuid = 0;
    schedule(cpuid);
    return 0;
}

int64_t sys_get_time(TimeVal *ts, uint64_t _tz) {
    ts->usec = get_time_us();
    ts->sec = ts->usec / MICRO_PER_SEC;
    return 0;
}

int64_t sys_exit(int32_t xstate) {
    int cpuid = 0;
    char buf[16];
    printk("[kernel] Process ");
    printk(itoa(cpus[cpuid].running->pid, buf));
    printk(" exited with code ");
    printk(itoa(xstate, buf));
    printk("\n");

    terminate(cpus[cpuid].running);
    unload_process(cpus[cpuid].running);
    schedule(cpuid);
    return 0;
}