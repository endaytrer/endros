#include <lib.h>

int main(int argc, const char *argv[]) {
    // page fault
    write(STDOUT, "Able to reach this line!\n", 26);
    int x = *(int *)(0);
    write(STDOUT, "Unable to reach this line!\n", 28);
    return x;
}