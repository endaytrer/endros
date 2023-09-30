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

void ls(FSFile *dir) {
    if (dir->type != DIRECTORY) {
        printk("[kernel] not a directory");
    }
    DirEntry *entries = kalloc(dir->size);
    fs_file_read(dir, 0, entries, dir->size);
    for (int i = 0; i < dir->size / sizeof(DirEntry); i++) {
        char buf[16];
        printk(itoa(entries[i].inode, buf));

        printk("\t");
        printk(entries[i].name);
        printk("\n");
    }
}