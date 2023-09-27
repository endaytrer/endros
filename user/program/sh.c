#include <lib.h>

void _start() {
    write(STDOUT, "sh> ", 5);
    char *buffer = sbrk(256);
    char *ptr = buffer;
    while (1) {
        char ch = 0;
        while (ch == 255 || ch == 0) {
            read(STDIN, &ch, 1);
        }
        if (ch == '\r') {
            if (strcmp(buffer, "exit") == 0) {
                exit(0);
            }
            int pid = fork();
            if (pid == 0) {
                exec(buffer);
            }
            waitpid(pid);
            memset(buffer, 0, 256);
            ptr = buffer;
            write(STDOUT, "\nsh> ", 6);
        } else {
            *ptr = ch;
            write(STDOUT, ptr, 2);
            ptr++;
        }
    }
}