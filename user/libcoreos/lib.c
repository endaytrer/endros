#include "lib.h"
#include "syscall.h"
int64_t write(uint64_t fd, const char *buffer, uint64_t size) {
    return sys_write(fd, buffer, size);
}
int64_t exit(int32_t xstate) {
    return sys_exit(xstate);
}