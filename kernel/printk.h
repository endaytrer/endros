#ifndef _K_PRINTK_H
#define _K_PRINTK_H
#include <stdint.h>
char *itoa(int64_t num, char *buffer);
void memset(void *start, uint8_t byte, uint64_t size);
void memcpy(void *dst, const void *src, uint64_t size);
void printk(const char *fmt);
void panic(const char *fmt);
#endif