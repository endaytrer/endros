#ifndef __LIB_H
#define __LIB_H
#include <type.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define NULL ((void *)0)
typedef struct {
    u64 sec;
    u64 usec;
} TimeVal;
typedef int pid_t;
i64 read(u64 fd, char *buffer, u64 size);
i64 write(u64 fd, const char *buffer, u64 size);
i64 exit(i32 xstate);
i64 yield(void);
u64 get_time();
void sleep(u64 time_us);
void *sbrk(i64 size);
pid_t fork(void);
i64 exec(const char *path);
i64 waitpid(pid_t pid);
i64 open(const char *path, int flags, int mode);
i64 close(int fd);
i64 lseek(int fd, i64 offset, u32 whence);
i64 chdir(const char *path);
void srand(u32 seed);
u32 rand(void);
#endif