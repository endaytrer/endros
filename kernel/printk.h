#ifndef _K_PRINTK_H
#define _K_PRINTK_H
#include <stdint.h>
void printk(const char *fmt);
void panic(const char *fmt);
#endif