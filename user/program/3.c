#include <lib.h>

void _start() {
    // page fault
    write(STDOUT, "Able to reach this line!\n", 26);
    int x = *(int *)(0);
    write(STDOUT, "Unable to reach this line!\n", 28);
    exit(0);
}