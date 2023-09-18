#include "lib.h"
#include "syscall.h"
int64_t write(uint64_t fd, const char *buffer) {
    return sys_write(fd, buffer);
}
int64_t exit(int32_t xstate) {
    return sys_exit(xstate);
}