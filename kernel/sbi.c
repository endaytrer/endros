#include "sbi.h"
void set_timer(u64 timer) {
    sbi_call(SBI_SET_TIMER, timer, 0, 0);
}
void kputc(char ch) {
    sbi_call(SBI_CONSOLE_PUTCHAR, ch, 0, 0);
}
char kgetc(void) {
    return sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}
void shutdown(void) {
    sbi_call(SBI_SHUTDOWN, 0, 0, 0);
}