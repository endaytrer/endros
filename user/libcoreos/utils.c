#define COUNTING_SYSTEM 10
#define DIGIT_TO_CHAR(i) ((i >= 10) ? ('a' + (i - 10)) : '0' + i)
#include "stdint.h"

char *itoa(int64_t num, char *buffer) {
    char *ptr = buffer;
    if (num < 0) {
        *(ptr++) = '-';
        num = -num;
    }
    if (COUNTING_SYSTEM == 16) {
        *(ptr++) = '0';
        *(ptr++) = 'x';
    } else if (COUNTING_SYSTEM == 8) {
        *(ptr++) = '0';
        *(ptr++) = 'o';
    } else if (COUNTING_SYSTEM == 2) {
        *(ptr++) = '0';
        *(ptr++) = 'b';
    }
    if (num == 0) {
        *(ptr++) = '0';
    }
    if (num == 1) {
        *(ptr++) = '1';
    }
    int p = 1;
    while (p <= num) {
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