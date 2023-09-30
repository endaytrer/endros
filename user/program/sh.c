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
            write(STDOUT, "\n", 2);
            if (strlen(buffer) == 0) {
                write(STDOUT, "sh> ", 6);
                continue;
            }
            if (strcmp(buffer, "exit") == 0) {
                exit(0);
            }
            int pid = fork();
            if (pid == 0) {
                exec(buffer);
                exit(-1);
            }
            if (pid > 0) {
                waitpid(pid);
            }
            memset(buffer, 0, 256);
            ptr = buffer;
            write(STDOUT, "sh> ", 6);
        } else {
            if (ch == 127) {
                if (ptr != buffer) {
                    write(STDOUT, "\033[1D \033[1D", 10);
                    *ptr-- = 0;
                }
            } else {

                *ptr = ch;
                write(STDOUT, ptr, 2);
                ptr++;
            }
        }
    }
}