#include <lib.h>

void _start(void) {
    // unknown instruction
    asm volatile(
        ".quad 0x0"
    );
    write(STDOUT, "Hello 4!\n", 10);
    exit(0);
}