#include "printk.h"
#include "sbi.h"
#define COUNTING_SYSTEM 16
#define DIGIT_TO_CHAR(i) (i >= 10 ? 'a' + i : '0' + i)


char *itoa(int64_t num, char *buffer) {
    char *ptr = buffer;
    if (num < 0) {
        *(ptr++) = '-';
        num = -num;
    }
    if (num == 0) {
        *(ptr++) = '0';
    }

    int p = 1;
    while (p < num) {
        p *= COUNTING_SYSTEM;
    }
    p /= COUNTING_SYSTEM;
    while (p != 0) {
        int k = num / p;
        num %= p;
        p /= COUNTING_SYSTEM;
        (*ptr++) = DIGIT_TO_CHAR(k);
    }
    *ptr = '\0';
    return buffer;
}

void printk(const char *fmt) {
    const char *ptr = fmt;
    while (*ptr != '\0') {
        kputc(*ptr++);
    }
}