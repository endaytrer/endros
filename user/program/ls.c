#include <lib.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    char *path;
    char default_path[] = ".";
    if (argc == 1) {
        path = default_path;
    } else if (argc == 2) {
        path = argv[1];
    } else {
        write(STDERR, "Usage: ls DIRECTORY\n", 21);
        return -1;
    }
    i64 fd;
    if ((fd = open(path, O_RDONLY | O_DIRECTORY, 0644)) < 0) {
        write(STDERR, "cannot open file\n", 18);
        return -1;
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
    return 0;
}
