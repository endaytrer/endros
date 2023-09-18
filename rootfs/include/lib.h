#ifndef __LIB_H
#define __LIB_H
#include <stdint.h>
#define STDIN 0
#define STDOUT 1
#define STDERR 2

int64_t write(uint64_t fd, const char *buffer);
int64_t exit(int32_t xstate);

#endif