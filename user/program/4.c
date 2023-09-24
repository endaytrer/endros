#include <lib.h>

void _start(void) {
    // unknown instruction
    asm volatile(
        ".quad 0xdeadbeef"
    );
    write(STDOUT, "Hello 4!\n");
    exit(0);
}