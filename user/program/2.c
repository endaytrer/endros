#include <lib.h>

void _start(void) {
    // unsupported syscall
    write(STDERR, "Hello 2!\n");
    exit(0);
}