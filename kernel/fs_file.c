#include "fs_file.h"
#include "filesystem.h"
#include "printk.h"
#include "mem.h"

i64 get_inode(const FSFile *file, Inode *out_inode) {
    u8 bitmap_byte;
    wrapped_read(file->fs->device,
        (u64)ADDR(file->fs->inode_bitmap_offset, BITMAP_OFFSET(file->inum)),
        &bitmap_byte, 1);
    if (!BITMAP_GET(bitmap_byte, file->inum)) {
        printk("[kernel] inode does not exist\n");
        return -1;
    }
    /// TODO: Maybe add inode cache support
    /// but i think since inodes are buffered in blocked device, it is unnecessary
    return wrapped_read(file->fs->device,
        (u64)ADDR(file->fs->inode_table_offset, file->inum * sizeof(Inode)),
        out_inode, sizeof(Inode));
}
i64 fs_file_init(FSFile *file) {
    Inode inode;
    if (get_inode(file, &inode) < 0) {
        return -1;
    }
    file->permission = inode.permission;
    file->size = inode.size_bytes;
    file->type = inode.type;
    return 0;
}
i64 fs_file_sync(const FSFile *file) {
    Inode inode;
    if (get_inode(file, &inode) < 0) {
        return -1;
    }
    inode.permission = file->permission;
    inode.size_bytes = file->size;
    // do not sync immutable type here
    return wrapped_write(file->fs->device,
        (u64)ADDR(file->fs->inode_table_offset, file->inum * sizeof(Inode)),
        &inode,
        sizeof(Inode));
}

void translate_fs(FSFile *file, Inode *inode, u64 offset, void *buffer, u64 size, bool write) {
    
    u64 page_offset = OFFSET(offset);
    u64 page_max = page_offset + size;

    for (int id = ADDR_2_PAGE(offset); id < ADDR_2_PAGEUP(offset + size); ++id) {
        u64 temp_page;
        if (page_max > PAGESIZE) {
            temp_page = PAGESIZE;
            page_max -= PAGESIZE;
        } else {
            temp_page = page_max;
        }
        u32 phy_page;
        const int ptr_per_page = PAGESIZE / sizeof(u32);
        if (id < DIRECT_PTRS) {
            phy_page = inode->direct[id];
        } else if (id < DIRECT_PTRS + ptr_per_page) {
            // single indirect
            id -= DIRECT_PTRS;
            wrapped_read(file->fs->device, ADDR(inode->single_ind, id * sizeof(u32)), &phy_page, sizeof(u32));
        } else if (id < DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page) {
            id -= DIRECT_PTRS + ptr_per_page;
            // (id >> 10) = id / (PAGESIZE / sizeof(u32)) is the index to indirect tables
            wrapped_read(file->fs->device, ADDR(inode->double_ind, id / ptr_per_page), &phy_page, sizeof(u32));
            wrapped_read(file->fs->device, ADDR(phy_page, id & (ptr_per_page - 1)), &phy_page, sizeof(u32));
        } else {
            id -= DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page;
            wrapped_read(file->fs->device, ADDR(inode->triple_ind, id / ptr_per_page / ptr_per_page), &phy_page, sizeof(u32));
            wrapped_read(file->fs->device, ADDR(phy_page, (id / ptr_per_page) & (ptr_per_page - 1)), &phy_page, sizeof(u32));
            wrapped_read(file->fs->device, ADDR(phy_page, id & (ptr_per_page - 1)), &phy_page, sizeof(u32));
        }
        i64 (*operation)(File *, u64, void *, u64) = write ?
            (i64 (*)(File *, u64, void *, u64))wrapped_write :
            wrapped_read;
        operation(file->fs->device, ADDR(phy_page, page_offset), buffer, temp_page - page_offset);

        buffer = (void *)((u64)buffer +  temp_page - page_offset);
        page_offset = 0;
    }
}

i64 fs_file_read(FSFile *file, u64 offset, void *buf, u64 size) {
    Inode inode;
    if (get_inode(file, &inode) < 0) {
        return -1;
    }
    translate_fs(file, &inode, offset, buf, size, false);
    return 0;
}

i64 fs_file_write(FSFile *file, u64 offset, const void *buf, u64 size) {
    Inode inode;
    if (get_inode(file, &inode) < 0) {
        return -1;
    }
    translate_fs(file, &inode, offset, (void *)buf, size, false);
    // since we do not change the file metadata, we do not have to sync.
    return 0;
}