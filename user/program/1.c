#include <lib.h>

void _start(void) {
    // write and exit
    write(STDOUT, "Hello 1!\n", 10);
    exit(0);
}