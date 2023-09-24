#include "sbi.h"
#include "printk.h"
#include "batch.h"
#include "trap.h"
#include "pagetable.h"

extern void sbss();
extern void ebss();

extern void kernel_stack_top();
extern void user_stack_top();

#define panic(info) \
do { \
    printk(info); \
    while (1) ; \
} while (0)

extern void _num_app(void);

void clear_bss(void) {
    for (uint8_t *ptr = (uint8_t *)sbss; ptr != (uint8_t *)ebss; ptr++) {
        *ptr = 0;
    }
}


static struct AppManager {
    uint32_t num_apps;
    uint32_t current_app;
    uint64_t app_start[16];
} app_manager;

void load_app(uint32_t appid) {
    if (appid > app_manager.num_apps) {
        printk("All applications are completed!\n");
        return;
    }
    memset(APP_BASE_ADDRESS, 0, APP_SIZE_LIMIT);
    memcpy(APP_BASE_ADDRESS, (const void *)(app_manager.app_start[appid]), app_manager.app_start[appid + 1] - app_manager.app_start[appid]);
    asm volatile(
        "fence.i"
    );
}

void run_next_app(void) {
    extern void __restore(uint64_t cx_addr);

    load_app(app_manager.current_app);
    app_manager.current_app += 1;

    // push context to kernel stack
    uint8_t *kernel_sp = (uint8_t *)kernel_stack_top;
    uint8_t *user_sp = (uint8_t *)user_stack_top;
    kernel_sp -= sizeof(TrapContext);
    app_init_context((TrapContext *)kernel_sp, APP_BASE_ADDRESS, (uint64_t)user_sp);
    __restore((uint64_t)kernel_sp);
    panic("Unreachable code!");
}

void init(void) {
    init_pagetable();
    clear_bss();

    // start app manager

    const uint64_t *ptr = (const uint64_t *)_num_app;
    uint64_t num_apps = *ptr;

    app_manager.num_apps = num_apps;
    app_manager.current_app = 0;
    ptr++;
    for (int i = 0; i <= num_apps; i++) {
        app_manager.app_start[i] = *(ptr + i);
    }
    char buf[64];
}

int main(void) {
    init();
    // load apps

    run_next_app();

    printk("\nDone!\n");
    return 0;
}

