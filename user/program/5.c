#include <lib.h>


void fibonacci(int k);

void _start(void) {
    fibonacci(5);
    write(STDOUT, "Hello 5!\n");
    exit(0);
}

void fibonacci(int k) {
    if (k == 0) return;
    char c[] = "0";
    c[0] += k;
    write(STDOUT, c);
    fibonacci(k - 1);
}