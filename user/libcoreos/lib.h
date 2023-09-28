#ifndef __LIB_H
#define __LIB_H
#include <type.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef struct {
    u64 sec;
    u64 usec;
} TimeVal;
typedef int pid_t;
i64 read(u64 fd, char *buffer, u64 size);
i64 write(u64 fd, const char *buffer, u64 size);
i64 exit(i32 xstate);
i64 yield(void);
i64 get_time(TimeVal *ts, u64 _tz);
void *sbrk(i64 size);
pid_t fork(void);
i64 exec(const char *path);
i64 waitpid(pid_t pid);

#endif