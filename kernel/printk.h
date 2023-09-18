#ifndef __PRINTK_H
#define __PRINTK_H
#include <stdint.h>
char *itoa(int64_t num, char *buffer);
void printk(const char *fmt);
#endif