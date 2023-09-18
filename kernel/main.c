#include <stdint.h>
#include "sbi.h"
#include "printk.h"
#include "batch.h"
extern void *sbss;
extern void *ebss;
#define panic(info) \
do { \
    printk(info); \
    while (1) ; \
} while (0)

void clear_bss(void) {
    for (uint8_t *ptr = (uint8_t *)sbss; ptr != (uint8_t *)ebss; ptr++) {
        *ptr = 0;
    }
}

int main(void) {
    clear_bss();
    char buf[64];
    printk("\nhello world!\n");
    printk(itoa((int64_t)app_manager, buf));
    
    volatile int a = app_manager->num_apps, b = 1;
    int c = a + b;
    return c;
}
