#include <lib.h>
#include <fcntl.h>

int main() {
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
            char *redirect_stdin = NULL;
            bool stdin_append = false;
            char *redirect_stdout = NULL;
            bool stdout_append = false;
            enum {
                Args,
                InFile,
                OutFile
            } state = Args;
            while (ptr != 0) {
                char *tok = strsep(&ptr, " ");
                if (*tok == '\0') continue;
                switch (state) {
                    case Args:
                        if (*tok == '>') {
                            if (strcmp(tok, ">>") == 0) {
                                stdout_append = true;
                                state = OutFile;
                            } else if (strcmp(tok, ">") == 0) {
                                stdout_append = false;
                                state = OutFile;
                            } else {
                                argv[argc++] = tok + 1; // treat as > something.
                                state = Args;
                            }
                        } else if (*tok == '<') {
                            if (strcmp(tok, "<<") == 0) {
                                stdin_append = true;
                                state = InFile;
                            } else if (strcmp(tok, "<") == 0) {
                                stdin_append = false;
                                state = InFile;
                            } else {
                                argv[argc++] = tok + 1; // treat as > something.
                                state = Args;
                            }
                        } else {
                            argv[argc++] = tok;
                            state = Args;
                        }
                        break;
                    case InFile:
                        redirect_stdin = tok;
                        state = Args;
                        break;
                    case OutFile:
                        redirect_stdout = tok;
                        state = Args;
                        break;
                }
            }
            if (state != Args) {
                write(STDERR, "usage: x </<</>/>> file\n", 25);
                goto reset;
            }
            if (strcmp(argv[0], "exit") == 0) {
                return 0;
            } else if (strcmp(argv[0], "cd") == 0) {
                if (argc != 2) {
                    write(STDERR, "usage: cd [DIRECTORY]\n", 23);
                    goto reset;
                } else if (chdir(argv[1]) < 0) {
                    write(STDERR, "cd failed\n", 11);
                    goto reset;
                }
            } else {
                int pid = fork();
                if (pid < 0) {
                    write(STDERR, "forked failed\n", 15);
                    goto reset;
                }
                if (pid > 0) {
                    waitpid(pid);
                } else {
                    argv[argc] = NULL;
                    if (redirect_stdin) {
                        int fd_in;
                        if ((fd_in = open(redirect_stdin, O_RDONLY | O_CREAT | (stdin_append ? O_APPEND : 0), 0644)) < 0) {
                            write(STDERR, "reopen stdin failed\n", 21);
                            exit(-1);
                        }
                        if (dup2(fd_in, STDIN) < 0) {
                            write(STDERR, "dup2 failed\n", 21);
                            exit(-1);
                        }
                        close(fd_in);
                    }

                    if (redirect_stdout) {
                        int fd_out;
                        if ((fd_out = open(redirect_stdout, O_WRONLY | O_CREAT | O_TRUNC | (stdout_append ? O_APPEND : 0), 0666)) < 0) {
                            write(STDERR, "reopen stdout failed\n", 21);
                            exit(-1);
                        }
                        if (dup2(fd_out, STDOUT) < 0) {
                            write(STDERR, "dup2 failed\n", 21);
                            exit(-1);
                        }
                        close(fd_out);
                    }
                    execv(argv[0], (const char *const *)argv);
                    return -1;
                }
            }
reset:
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