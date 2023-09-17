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
    printk("hello world!\n");
    volatile int a = 0, b = 1;
    int c = a + b;
    return c;
}
