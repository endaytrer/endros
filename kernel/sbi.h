#ifndef __SBI_H
#define __SBI_H
#include <stdint.h>
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

/* sbi_call: Call Risc-V SBI interrupt. SBI_* defines the sbi number. */
static inline __attribute__((always_inline)) uint64_t sbi_call(uint64_t sbi, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    register uint64_t x10 asm("x10") = arg0;
    register uint64_t x11 asm("x11") = arg1;
    register uint64_t x12 asm("x12") = arg2;
    register uint64_t x17 asm("x17") = sbi;
    asm volatile(
        "ecall\n"
        : "+r" (x10)
        : "r"(x10), "r" (x11), "r" (x12), "r" (x17)
    );
    return x10;
}
/* kputc: print a character on console. */
void kputc(char);
/* shutdown: shutdown machine */
void shutdown(void);
#endif