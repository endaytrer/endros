#ifndef _K_BATCH_H
#define _K_BATCH_H
#include <stdint.h>
#include "pagetable.h"
#define USER_STACK_SIZE 0x100000
#define KERNEL_STACK_SIZE 0x100000
#define APP_SIZE_LIMIT 0x20000


typedef struct {
    pfn_t ptbase_pfn;
    vpn_t ptbase_vpn;

    PTReference *ptref_base; // kallocked
    uint64_t num_sections;
    struct {
        pfn_t pfn;
        vpn_t vpn;
    } sections[APP_SIZE_LIMIT / PAGESIZE];

    struct {
        pfn_t pfn;
        vpn_t vpn;
    } user_stack[USER_STACK_SIZE / PAGESIZE];
    pfn_t trap_pfn;
    vpn_t trap_vpn;
    void *kernel_stack;
} Task;

typedef struct {
    uint32_t num_apps;
    uint32_t current_app;
    uint64_t app_start[16];
    Task current_task;
} AppManager;

void init_app_manager(void);
void run_next_app(void);
void load_app(Task *task, uint32_t appid);
#endif