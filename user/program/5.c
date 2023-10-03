#include <lib.h>

void fibonacci(int k) {
    if (k == 0) return;
    write(STDOUT, "5", 2);
    for (volatile int i = 0; i < 1000000; i++) ;
    fibonacci(k - 1);
}

int main(int argc, const char *argv[]) {
    fibonacci(100);
    write(STDOUT, "\nHello 5!\n", 10);
    return 0;
}
