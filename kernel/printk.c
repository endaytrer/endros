#include "printk.h"
#include "sbi.h"
void printk(const char *fmt) {
    const char *ptr = fmt;
    while (*ptr != '\0') {
        kputc(*ptr++);
    }
}