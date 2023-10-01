#include <lib.h>
#include <fcntl.h>

void _start() {
    write(STDOUT, "sh> ", 5);
    char *buffer = sbrk(4096);
    char *ptr = buffer;
    char *argv[64];

    while (1) {
        char ch = 0;
        while (ch == 255 || ch == 0) {
            lseek(STDIN, 0, SEEK_SET);
            read(STDIN, &ch, 1);
        }
        if (ch == '\r') {
            write(STDOUT, "\n", 2);
            if (strlen(buffer) == 0) {
                write(STDOUT, "sh> ", 6);
                continue;
            }
            // tokenize string
            ptr = buffer;
            int argc = 0;
            while (ptr != 0) {
                char *tok = strsep(&ptr, " ");
                if (*tok == '\0') continue;
                argv[argc++] = tok;
            }

            if (strcmp(argv[0], "exit") == 0) {
                exit(0);
            } else if (strcmp(argv[0], "cd") == 0) {
                if (argc != 2) {
                    write(STDERR, "usage: cd [DIRECTORY]\n", 23);
                } else {
                    chdir(argv[1]);
                }
            } else {
                int pid = fork();
                if (pid < 0) {
                    write(STDERR, "forked failed", 6);
                }
                if (pid > 0) {
                    waitpid(pid);
                } else {
                    exec(argv[0]);
                    exit(-1);
                }
            }
            memset(buffer, 0, 4096);
            ptr = buffer;
            write(STDOUT, "sh> ", 6);
        } else {
            if (ch == 127) {
                if (ptr != buffer) {
                    write(STDOUT, "\033[1D \033[1D", 10);
                    *(--ptr) = 0;
                }
            } else {

                *ptr = ch;
                write(STDOUT, ptr, 2);
                ptr++;
            }
        }
    }
}