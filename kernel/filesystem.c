#include <string.h>
#include "drivers/virtio_blk.h"
#include "drivers/virtio_gpu.h"
#include "filesystem.h"
#include "fs_file.h"
#include "pagetable.h"
#include "printk.h"
#include "machine_spec.h"
Filesystem rootfs;

static VirtIOBlk virtio_blk;
static BufferedBlockDevice root_block_buffered_dev;
static File root_block_device;
static VirtIOGPU virtio_gpu;
void init_filesystem(void) {
    // find a loadable disk, the one with smallest address
    VirtIOHeader *blk_header = (VirtIOHeader *)(~((u64)0));
    VirtIOHeader *gpu_header = (VirtIOHeader *)(~((u64)0));
    for (int i = 0; i < num_virtio_mmio; i++) {
        if (virtio_mmio_headers[i]->device_id == VIRTIO_DEVICE_BLK &&
                (u64)virtio_mmio_headers[i] < (u64)blk_header) {
            blk_header = virtio_mmio_headers[i];
        }
        if (virtio_mmio_headers[i]->device_id == VIRTIO_DEVICE_GPU &&
            (u64)virtio_mmio_headers[i] < (u64)gpu_header) {
            gpu_header = virtio_mmio_headers[i];
        }
    }

    if (blk_header == (VirtIOHeader *)(~((u64)0))) {
        panic("[kernel] Cannot find loadable disk\n");
    }
    if (gpu_header != (VirtIOHeader *)(~((u64)0))) {
        init_virtio_gpu(&virtio_gpu, gpu_header);
    }

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