#include <lib.h>
#include <fcntl.h>

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        write(STDERR, "Usage: cat FILE\n", 17);
        return -1;
    }
    int fd = open(argv[1], O_RDONLY, 0644);
    if (fd == -1) {
        write(STDERR, "cannot open file\n", 18);
        return -1;
    }
    char ch;
    while (read(fd, &ch, 1) == 0) {
        write(STDOUT, &ch, 1);
    }
    return 0;
}