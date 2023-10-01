#include <lib.h>

void _start(void) {
    // write and exit
    write(STDOUT, "Hello 1!\n", 10);

    i64 *s = sbrk(sizeof(i64));
    *s = 16;
    char buf[16];
    write(STDOUT, itoa(*s, buf, 10), 16);
    write(STDOUT, "\n", 2);
    int pid = fork();
    if (pid < 0) {
        write(STDOUT, "Fork failed!\n", 14);
    }
    if (pid == 0) {
        exec("/bin/5");
    }
    for (volatile int i = 0; i < 100000000; i++) {
        if (i % 1000000 == 0) {
            write(STDOUT, "1", 2);
        }
    }
    waitpid(pid);
    write(STDOUT, "Hello end!\n", 10);
    exit(0);
}