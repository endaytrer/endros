#include <lib.h>

void _start() {
    char str[] = "Hello world!\n";
    write(STDOUT, str, sizeof(str));
    pid_t pid = fork();
    if (pid < 0) {
        write(STDOUT, "fork failed\n", 12);
    }
    if (pid == 0) {
        // child process
        write(STDOUT, "I'm the child!\n", 15);
    } else {
        write(STDOUT, "I'm the parent!\n", 16);
    }
    exit(0);
}