#include "filesystem.h"
#include "fs_file.h"
#include "pagetable.h"

#include "printk.h"
#include <string.h>

Filesystem rootfs;

void init_filesystem(Filesystem *fs, File *dev) {
    // read super block (immutable)
    fs->device = dev;
    wrapped_read(dev, 0, &fs->super, sizeof(SuperBlock));

    fs->inode_bitmap_offset = 1;
    fs->data_bitmap_offset = 1 + fs->super.inode_bitmap_blocks;
    fs->inode_table_offset = 1 + fs->super.inode_bitmap_blocks + fs->super.data_bitmap_blocks;

    fs->root.fs = fs;
    fs->root.inum = fs->super.root_inode;
    fs_file_init(&fs->root);
}

i64 getfile(FSFile *cwd, const char *path, FSFile *fileout) {

    const char *ptr = path;
    if (*ptr == '/') {
        cwd = &cwd->fs->root;
        ptr++;
    }
    if (*ptr == '\0') {
        printk("[kernel] getting null file\n");
        return -1;
    }
    const char *end = ptr;
    while (*end != '\0') {
        if (cwd->type != DIRECTORY) {
            printk("[kernel] not a directory");
            return -1;
        }
        // get file / directory name
        for (; *end != '\0' && *end != '/'; end++);

        u64 name_size = end - ptr;
        // find the inode of entry
        DirEntry *entries = kalloc(cwd->size);
        fs_file_read(cwd, 0, entries, cwd->size);
        u64 dir_size = cwd->size;
        bool found = false;
        for (int i = 0; i < dir_size / sizeof(DirEntry); i++) {
            if (strlen(entries[i].name) == name_size && strncmp(ptr, entries[i].name, name_size) == 0) {

                fileout->fs = cwd->fs;
                fileout->inum = entries[i].inode;

                fs_file_init(fileout);

                found = true;
                break;

            }
        }
        kfree(entries, dir_size);
        if (!found) {
            printk("[kernel] cannot find item named ");
            printk(ptr);
            printk(" in path ");
            printk(path);
            printk("\n");
            return -1;
        }
        cwd = fileout;
        if (*end == '/') {
            end++;
        }
        ptr = end;
    }
    return 0;
}