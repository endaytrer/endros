#include <string.h>
#include "drivers/virtio_blk.h"
#include "drivers/virtio_gpu.h"
#include "filesystem.h"
#include "fs_file.h"
#include "pagetable.h"
#include "printk.h"
#include "machine_spec.h"
#include "stdio.h"

Filesystem rootfs;

static VirtIOBlk virtio_blk;
static BufferedBlockDevice root_block_buffered_dev;
static File root_block_device;
VirtIOGPU virtio_gpu;
File gpu_device;


void init_filesystem(VirtIOHeader *blk_header) {
    // find a loadable disk, the one with smallest address
    init_virtio_blk(&virtio_blk, blk_header);
    wrap_virtio_blk_device(&root_block_buffered_dev, &virtio_blk);
    wrap_block_buffer_file(&root_block_device, &root_block_buffered_dev);
    create_filesystem(&rootfs, &root_block_device);
}
void create_filesystem(Filesystem *fs, File *dev) {

    // read super block (immutable)
    fs->device = dev;
    wrapped_read(dev, 0, &fs->super, sizeof(SuperBlock));

    fs->inode_bitmap_offset = 1;
    fs->data_bitmap_offset = 1 + fs->super.inode_bitmap_blocks;
    fs->inode_table_offset = 1 + fs->super.inode_bitmap_blocks + fs->super.data_bitmap_blocks;

    fs->root.fs = fs;
    fs->root.inum = fs->super.root_inode;
    fs_file_init(&fs->root);
    File outfile;
    getfile(&fs->root, "/dev", &outfile);
    memcpy(&fs->dev, outfile.super, sizeof(FSFile));
    kfree(outfile.super, sizeof(FSFile));
}
/// @brief Get a file / dev.
/// @param cwd
/// @param path 
/// @param file_out output file
/// @return t
i64 getfile(FSFile *cwd, const char *path, File *out) {

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

    FSFile *fileout = kalloc(sizeof(FSFile));
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
        if (cwd->inum == cwd->fs->dev.inum) {
            /// TODO: this is a temporary fix. When filesystem is not readonly anymore, mount devices to correspondent files.
            // if it is device, just return this.
            if (strcmp(ptr, "framebuffer") == 0) {
                kfree(fileout, sizeof(FSFile));
                memcpy(out, &gpu_device, sizeof(File));
                return 0;
            } else if (strcmp(ptr, "stdin") == 0) {
                kfree(fileout, sizeof(FSFile));
                memcpy(out, &stdin, sizeof(File));
                return 0;
            } else if (strcmp(ptr, "stdout") == 0) {
                kfree(fileout, sizeof(FSFile));
                memcpy(out, &stdout, sizeof(File));
                return 0;
            } else if (strcmp(ptr, "stderr") == 0) {
                kfree(fileout, sizeof(FSFile));
                memcpy(out, &stdout, sizeof(File));
                return 0;
            } else if (strcmp(ptr, "vda") == 0) {
                kfree(fileout, sizeof(FSFile));
                memcpy(out, rootfs.device, sizeof(File));
                return 0;
            }
        }
        kfree(entries, dir_size);
        if (!found) {
            printk("[kernel] cannot find item named ");
            printk(ptr);
            printk(" in path ");
            printk(path);
            printk("\n");
            kfree(fileout, sizeof(FSFile));
            return -1;
        }
        cwd = fileout;
        if (*end == '/') {
            end++;
        }
        ptr = end;
    }
    wrap_fsfile(out, fileout);
    return 0;
}