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

int strncmp(const char *s1, const char *s2, u64 size) {
    u64 i = 0;
    for (; i < size && *s1 != 0 && *s2 != 0; i++) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    if (i == size) return 0;
    return *s1 - *s2;
}

u64 strlen(const char *s) {
    u64 ans = 0;
    while (s[ans] != 0) ans++;
    return ans;
}

char *strsep(char **stringp, const char *delim) {
    ///TODO: brute force approach, maybe replace it with KMP
    u64 len = strlen(delim);
    for (char *ptr = *stringp; *(ptr + len) != '\0'; ++ptr) {
        if (strncmp(ptr, delim, len) != 0) {
            continue;
        }
        for (int i = 0; i < len; i++) {
            *(ptr + i) = '\0';
        }
        char *temp = *stringp;
        *stringp = ptr + len;
        return temp;
    }
    // if no token is seperated, set to null;
    char *temp = *stringp;
    *stringp = (char *)0;
    return temp;
}

void memset(void *start, u8 byte, u64 size) {
    for (u8 *ptr = (u8 *)start; ptr < (u8 *)start + size; ++ptr) {
        *ptr = byte;
    }
}

void memcpy(void *dst, const void *src, u64 size) {
    for (u64 i = 0; i < size; i++) {
        *((u8 *)dst + i) = *((u8 *)src + i);
    }
}
char *itoa(i64 num, char *buffer, int counting_system) {
    char *ptr = buffer;
    if (num < 0) {
        *(ptr++) = '-';
        num = -num;
    }
    if (counting_system == 16) {
        *(ptr++) = '0';
        *(ptr++) = 'x';
    } else if (counting_system == 8) {
        *(ptr++) = '0';
        *(ptr++) = 'o';
    } else if (counting_system == 2) {
        *(ptr++) = '0';
        *(ptr++) = 'b';
    }
    if (num == 0) {
        *(ptr++) = '0';
    }
    i64 p = 1;
    while (p <= num) {
        p *= counting_system;
    }
    p /= counting_system;
    while (p != 0) {
        int k = num / p;
        num %= p;
        p /= counting_system;
        (*ptr++) = DIGIT_TO_CHAR(k);
    }
    *ptr = '\0';
    return buffer;
}