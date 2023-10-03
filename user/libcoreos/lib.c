#include "lib.h"
#include "syscall.h"
i64 read(u64 fd, char *buffer, u64 size) {
    return sys_read(fd, buffer, size);
}
i64 write(u64 fd, const char *buffer, u64 size) {
    return sys_write(fd, buffer, size);
}

i64 exit(i32 xstate) {
    return sys_exit(xstate);
}

i64 yield(void) {
    return sys_yield();
}

u64 get_time() {
    TimeVal ts;
    sys_get_time(&ts, 0);
    return ts.usec;
}

void *sbrk(i64 size) {
    return (void *)sys_sbrk(size);
}

pid_t fork(void) {
    return sys_fork();
}

i64 exec(const char *path) {
    return sys_exec(path);
}

i64 waitpid(pid_t pid) {
    return sys_waitpid(pid);
}

i64 open(const char *path, int flags, int mode) {
    return sys_openat(0, path, flags, mode);
}

i64 close(int fd) {
    return sys_close(fd);
}

i64 lseek(int fd, i64 offset, u32 whence) {
    u64 res;
    int ret = sys_lseek(fd, 0, offset, &res, whence);
    if (ret < 0) return ret;
    return res;
}
i64 chdir(const char *path) {
    return sys_chdir(path);
}

u32 rand_seed = 0xdeadbeef;
void srand(u32 seed) {
    rand_seed = seed;
}
u32 rand(void) {
    rand_seed = rand_seed % (0xc12c9d07) + 0x123c6dfc;
    rand_seed = rand_seed * (rand_seed - 0x812c9d07) + 0x123746;
    return rand_seed;
}

void sleep(u64 time_us) {
    TimeVal time;
    sys_get_time(&time, 0);
    u64 start = time.usec;
    do {
        yield();
        sys_get_time(&time, 0);
    } while (time.usec - start < time_us);
}