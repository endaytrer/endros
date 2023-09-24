#ifndef _K_SYSCALL_H
#define _K_SYSCALL_H
#include <stdint.h>


#define SYS_WRITE 64
#define SYS_EXIT 93

int64_t sys_write(uint64_t fd, uint8_t *buf, uint64_t size);
int64_t sys_exit(int32_t xstate);


int64_t syscall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2);

#endif