#ifndef __LIB_H
#define __LIB_H
#include <stdint.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef struct {
    uint64_t sec;
    uint64_t usec;
} TimeVal;
typedef int pid_t;
int64_t write(uint64_t fd, const char *buffer, uint64_t size);
int64_t exit(int32_t xstate);
int64_t yield(void);
int64_t get_time(TimeVal *ts, uint64_t _tz);
void *sbrk(int64_t size);
pid_t fork(void);
int64_t exec(const char *path);
int64_t waitpid(pid_t pid);

#endif