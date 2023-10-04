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
    getfile(&fs->root, "/dev", &outfile, false);
    memcpy(&fs->dev, outfile.super, sizeof(FSFile));
    kfree(outfile.super, sizeof(FSFile));
}
/// @brief Get a file / dev.
/// @param cwd
/// @param path 
/// @param file_out output file
/// @return t
i64 getfile(FSFile *cwd, const char *path, File *out, bool create) {

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
        if (cwd->inode.type != DIRECTORY) {
            printk("[kernel] not a directory");
            return -1;
        }
        // get file / directory name
        for (; *end != '\0' && *end != '/'; end++);

        u64 name_size = end - ptr;
        // find the inode of entry
        DirEntry *entries = kalloc(cwd->inode.size_bytes);
        fs_file_read(cwd, 0, entries, cwd->inode.size_bytes);
        u64 dir_size = cwd->inode.size_bytes;
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
        if (*end == '\0' && !found && create) {
            return create_file(cwd, ptr, out);
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
i64 allocate_inode(Filesystem *fs) {
    // get a free inode, also mark it as not free.
    u64 inode_bitmap_offset = 1;
    for (u32 offset = 0; offset < BITMAP_OFFSET(fs->super.size_blocks) + (BITMAP_BITS(fs->super.size_blocks) ? 1 : 0); offset++) {
        // we know what we are doing exactly here, so we are not using wrapped_read
        u8 buf;
        fs->device->read(fs->device->super, ADDR(inode_bitmap_offset, offset), &buf, 1);
        if (buf == 0xff)
            continue;
        for (int bit = 0; bit < 8; bit++) {
            if (BITMAP_GET(buf, bit) != 0)
                continue;

            BITMAP_SET(&buf, bit);
            fs->device->write(fs->device->super, ADDR(inode_bitmap_offset, offset), &buf, 1);
            return BITMAP_2_ADDR(offset, bit);
        }
    }
    return -1;
}
i64 free_inode(Filesystem *fs, u32 inum) {
    u64 inode_bitmap_offset = 1;
    char buf;
    wrapped_read(fs->device, ADDR(inode_bitmap_offset, BITMAP_OFFSET(inum)), &buf, 1);
    if (BITMAP_GET(buf, inum) == 0)
        return -1;
    BITMAP_CLEAR(&buf, inum);
    wrapped_write(fs->device, ADDR(inode_bitmap_offset, BITMAP_OFFSET(inum)), &buf, 1);
    return 0;
}
i64 allocate_block(Filesystem *fs) {
    // get a free block, also mark it as not free.
    u64 data_bitmap_offset = 1 + fs->super.inode_bitmap_blocks;
    for (u32 offset = 0; offset < BITMAP_OFFSET(fs->super.size_blocks) + (BITMAP_BITS(fs->super.size_blocks) ? 1 : 0); offset++) {
        // we know what we are doing exactly here, so we are not using wrapped_read
        u8 buf;
        fs->device->read(fs->device->super, ADDR(data_bitmap_offset, offset), &buf, 1);
        if (buf == 0xff)
            continue;
        for (int bit = 0; bit < 8; bit++) {
            if (BITMAP_GET(buf, bit) != 0)
                continue;

            BITMAP_SET(&buf, bit);
            fs->device->write(fs->device->super, ADDR(data_bitmap_offset, offset), &buf, 1);
            return BITMAP_2_ADDR(offset, bit);
        }
    }
    return -1;
}
i64 free_block(Filesystem *fs, u32 block_id) {
    u64 data_bitmap_offset = 1 + fs->super.inode_bitmap_blocks;
    char buf;
    wrapped_read(fs->device, ADDR(data_bitmap_offset, BITMAP_OFFSET(block_id)), &buf, 1);
    if (BITMAP_GET(buf, block_id) == 0)
        return -1;
    BITMAP_CLEAR(&buf, block_id);
    wrapped_write(fs->device, ADDR(data_bitmap_offset, BITMAP_OFFSET(block_id)), &buf, 1);
    return 0;

}
/// Creating a zero-byte regular file with RW permission
/// \param parent
/// \param filename
/// \param file
/// \return
i64 create_file(FSFile *parent, const char *filename, File *file) {

    u64 old_size = parent->inode.size_bytes;
    if (fs_file_truncate(parent, old_size + sizeof(DirEntry)) < 0) {
        return -1;
    }
    // allocate new inode
    DirEntry dir_entry;
    dir_entry.inode = allocate_inode(parent->fs);
    if (dir_entry.inode< 0) {
        fs_file_truncate(parent, old_size);
        return -1;
    }
    u64 len = strlen(filename);
    if (sizeof(dir_entry.name) < len)
        len = sizeof(dir_entry.name);

    memcpy(dir_entry.name, filename, len);
    fs_file_write(parent, old_size, &dir_entry, sizeof(DirEntry));

    // write the inode
    FSFile *fsfile = kalloc(sizeof(FSFile));
    fsfile->fs = parent->fs;
    fsfile->inum = dir_entry.inode;
    fsfile->rc = 0;
    fsfile->inode.size_bytes = 0;
    fsfile->inode.type = FILE;
    fsfile->inode.permission = PERMISSION_R | PERMISSION_W;

    fs_file_sync(fsfile);
    wrap_fsfile(file, fsfile);
    return 0;
}