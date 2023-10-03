#include "syscall.h"

i64 syscall(u64 id, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6) {
    register u64 x10 asm("x10") = arg0;
    register u64 x11 asm("x11") = arg1;
    register u64 x12 asm("x12") = arg2;
    register u64 x13 asm("x13") = arg3;
    register u64 x14 asm("x14") = arg4;
    register u64 x15 asm("x15") = arg5;
    register u64 x16 asm("x16") = arg6;
    register u64 x17 asm("x17") = id;
    asm volatile("ecall"
    : "=r"(x10)
    : "r"(x10),"r"(x11),"r"(x12),"r"(x13),"r"(x14),"r"(x15),"r"(x16),"r"(x17)
    );
    return x10;
}