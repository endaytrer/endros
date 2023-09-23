#include <stdint.h>
#include "sbi.h"
#include "printk.h"
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
    return 0;
}
