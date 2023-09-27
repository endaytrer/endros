#include <lib.h>

void _start(void) {
    // write and exit
    write(STDOUT, "Hello 1!\n", 10);
    for (volatile int i = 0; i < 100000000; i++) {
        if (i % 1000000 == 0) {
            write(STDOUT, "1", 2);
        }
    }
    write(STDOUT, "Hello end!\n", 10);
    exit(0);
}