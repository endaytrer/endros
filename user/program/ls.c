#include <lib.h>
#include <fcntl.h>

void _start() {

    i64 fd;
    if ((fd = open(".", O_RDONLY | O_DIRECTORY, 0644)) < 0) {
        write(STDERR, "cannot open file\n", 18);
        exit(-1);
    }
    struct dir_entry {
        char name[60];
        int inode;
    } entry;
    while (read(fd, (char *)&entry, sizeof(entry)) >= 0) {
        char buf[16];
        write(STDOUT, itoa(entry.inode, buf, 16), 16);
        write(STDOUT, "\t", 2);
        write(STDOUT, entry.name, 60);
        write(STDOUT, "\n", 2);
    }
    exit(0);
}
