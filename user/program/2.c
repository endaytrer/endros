#include <lib.h>

int main(int argc, const char *argv[]) {
    // unsupported syscall
    write(STDERR, "Hello 2!\n", 10);
    return 0;
}