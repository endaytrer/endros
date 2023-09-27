#define COUNTING_SYSTEM 10
#define DIGIT_TO_CHAR(i) ((i >= 10) ? ('a' + (i - 10)) : '0' + i)
#include "string.h"

int strcmp(const char *s1, const char *s2) {
    while (*s1 != 0 && *s2 != 0) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

void memset(void *start, uint8_t byte, uint64_t size) {
    for (uint8_t *ptr = (uint8_t *)start; ptr < (uint8_t *)start + size; ++ptr) {
        *ptr = byte;
    }
}

void memcpy(void *dst, const void *src, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        *((uint8_t *)dst + i) = *((uint8_t *)src + i);
    }
}

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