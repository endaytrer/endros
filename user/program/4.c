#include <lib.h>

int main(int argc, const char *argv[]) {
    // unknown instruction
    asm volatile(
        ".quad 0x0"
    );
    write(STDOUT, "Hello 4!\n", 10);
    return 0;
}