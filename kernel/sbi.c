#include "sbi.h"

void kputc(char ch) {
    sbi_call(SBI_CONSOLE_PUTCHAR, ch, 0, 0);
}

void shutdown(void) {
    sbi_call(SBI_SHUTDOWN, 0, 0, 0);
}