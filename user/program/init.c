#include <lib.h>

int main(int argc, const char *argv[]) {
    char str[] = "Hello world!\n";
    write(STDOUT, str, sizeof(str));
    pid_t pid = fork();
    if (pid < 0) {
        write(STDOUT, "fork failed\n", 12);
    }
    if (pid == 0) {
        // child process
        write(STDOUT, "I'm the child!\n", 15);
        exec("/bin/sh");
    } else {
        waitpid(pid);
        write(STDOUT, "Child has exited\n", 18);
    }
    return 0;
}