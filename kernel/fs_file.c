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
i64 fs_file_sync(const FSFile *file, const Inode *inode) {
    return wrapped_write(file->fs->device,
        (u64)ADDR(file->fs->inode_table_offset, file->inum * sizeof(Inode)),
        inode,
        sizeof(Inode));
}

void translate_fs(FSFile *file, u64 offset, void *buffer, u64 size, bool write) {
    
    u64 page_offset = OFFSET(offset);
    u64 page_max = page_offset + size;
    Inode inode;
    get_inode(file, &inode);

    for (u64 id = ADDR_2_PAGE(offset); id < ADDR_2_PAGEUP(offset + size); ++id) {
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
            phy_page = inode.direct[id];
        } else if (id < DIRECT_PTRS + ptr_per_page) {
            // single indirect
            u64 read_id = id - DIRECT_PTRS;
            wrapped_read(file->fs->device, ADDR(inode.single_ind, read_id * sizeof(u32)), &phy_page, sizeof(u32));
        } else if (id < DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page) {
            u64 read_id = id - (DIRECT_PTRS + ptr_per_page);
            // (id >> 10) = id / (PAGESIZE / sizeof(u32)) is the index to indirect tables
            wrapped_read(file->fs->device, ADDR(inode.double_ind, read_id / ptr_per_page), &phy_page, sizeof(u32));
            wrapped_read(file->fs->device, ADDR(phy_page, read_id & (ptr_per_page - 1)), &phy_page, sizeof(u32));
        } else {
            u64 read_id = id - (DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page);
            wrapped_read(file->fs->device, ADDR(inode.triple_ind, read_id / ptr_per_page / ptr_per_page), &phy_page, sizeof(u32));
            wrapped_read(file->fs->device, ADDR(phy_page, (read_id / ptr_per_page) & (ptr_per_page - 1)), &phy_page, sizeof(u32));
            wrapped_read(file->fs->device, ADDR(phy_page, read_id & (ptr_per_page - 1)), &phy_page, sizeof(u32));
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
    translate_fs(file, offset, buf, size, false);
    return 0;
}

i64 fs_file_write(FSFile *file, u64 offset, const void *buf, u64 size) {
    translate_fs(file, offset, (void *)buf, size, true);
    return 0;
}

i64 fs_file_truncate(FSFile *file, u64 newsize) {
    Inode inode;
    get_inode(file, &inode);
    u64 oldsize = inode.size_bytes;
    if (oldsize == newsize) return 0;
    u64 old_pages = ADDR_2_PAGEUP(oldsize);
    u64 new_pages = ADDR_2_PAGEUP(newsize);
    const int ptr_per_page = PAGESIZE / sizeof(u32);
    if (old_pages < new_pages) {
        // allocate new pages
        i64 block_1st, block_2nd, block_3rd, temp_block;
        for (;old_pages < new_pages; old_pages++) {
            if (old_pages < DIRECT_PTRS) {
                if ((temp_block = allocate_block(file->fs)) < 0)
                    return -1;
                inode.direct[old_pages] = temp_block;

            } else if (old_pages < DIRECT_PTRS + ptr_per_page) {
                u64 id = old_pages - DIRECT_PTRS;
                if (id == 0) {
                    if ((block_1st = allocate_block(file->fs)) < 0)
                        return -1;
                    inode.single_ind = block_1st;
                }
                if ((temp_block = allocate_block(file->fs)) < 0)
                    return -1;
                wrapped_write(file->fs->device, ADDR(inode.single_ind, id * sizeof(u32)), &temp_block, sizeof(u32));
            } else if (old_pages < DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page) {
                u64 id = old_pages - (DIRECT_PTRS + ptr_per_page);
                if (id == 0) {
                    if ((block_1st = allocate_block(file->fs)) < 0)
                        return -1;
                    inode.double_ind = block_1st;
                }
                if (id % ptr_per_page == 0) {
                    // the first of second level
                    if ((block_2nd = allocate_block(file->fs)) < 0)
                        return -1;
                    wrapped_write(file->fs->device, ADDR(inode.double_ind, id / ptr_per_page), &block_2nd, sizeof(u32));
                }
                // if second level page is just allocated, block is set to second level block. 
                // Otherwise, since (id == 0 && id % ptr_per_page != 0) == false, it is just 0.

                if (!block_2nd) 
                    wrapped_read(file->fs->device, ADDR(inode.double_ind, id / ptr_per_page), &block_2nd, sizeof(u32));

                if ((temp_block = allocate_block(file->fs)) < 0)
                    return -1;
                wrapped_write(file->fs->device, ADDR(block_2nd, id % ptr_per_page), &temp_block, sizeof(u32));
            } else {
                u64 id = old_pages - (DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page);
                if (id == 0) {
                    if ((block_1st = allocate_block(file->fs)) < 0)
                        return -1;
                    inode.triple_ind = block_1st;
                }
                if (id % (ptr_per_page * ptr_per_page) == 0) {
                    if ((block_2nd = allocate_block(file->fs)) < 0)
                        return -1;
                    wrapped_write(file->fs->device, ADDR(inode.triple_ind, id / ptr_per_page / ptr_per_page), &block_2nd, sizeof(u32));
                }
                if (!block_2nd) 
                    wrapped_read(file->fs->device, ADDR(inode.triple_ind, id / ptr_per_page / ptr_per_page), &block_2nd, sizeof(u32));

                if (id % ptr_per_page == 0) {
                    if ((block_3rd = allocate_block(file->fs)) < 0)
                        return -1;
                    wrapped_write(file->fs->device, ADDR(block_2nd, (id / ptr_per_page) % ptr_per_page), &block_3rd, sizeof(u32));
                }
                if (!block_3rd)
                    wrapped_read(file->fs->device, ADDR(block_2nd, (id / ptr_per_page) % ptr_per_page), &block_3rd, sizeof(u32));
                
                if ((temp_block = allocate_block(file->fs)) < 0)
                    return -1;
                wrapped_write(file->fs->device, ADDR(block_3rd, id % ptr_per_page), &temp_block, sizeof(u32));
            }
        }
    } else if (old_pages > new_pages) {
        for (u64 id = old_pages - 1; id >= new_pages; id--) {
            u32 block_2nd, block_3rd, temp_block;
            if (id < DIRECT_PTRS) {
                free_block(file->fs, inode.direct[id]);
            } else if (id < DIRECT_PTRS + ptr_per_page) {
                // single indirect
                u64 read_id = id - DIRECT_PTRS;
                wrapped_read(file->fs->device, ADDR(inode.single_ind, read_id * sizeof(u32)), &temp_block, sizeof(u32));
                free_block(file->fs, temp_block);
                if (read_id == 0) 
                    free_block(file->fs, inode.single_ind);

            } else if (id < DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page) {
                u64 read_id = id - (DIRECT_PTRS + ptr_per_page);
                // (id >> 10) = id / (PAGESIZE / sizeof(u32)) is the index to indirect tables
                wrapped_read(file->fs->device, ADDR(inode.double_ind, read_id / ptr_per_page), &block_2nd, sizeof(u32));
                wrapped_read(file->fs->device, ADDR(block_2nd, read_id & (ptr_per_page - 1)), &temp_block, sizeof(u32));
                free_block(file->fs, temp_block);

                if (read_id % ptr_per_page == 0)
                    free_block(file->fs, block_2nd);

                if (read_id == 0)
                    free_block(file->fs, inode.double_ind);
                
            } else {
                u64 read_id = id - (DIRECT_PTRS + ptr_per_page + ptr_per_page * ptr_per_page);
                wrapped_read(file->fs->device, ADDR(inode.triple_ind, read_id / ptr_per_page / ptr_per_page), &block_2nd, sizeof(u32));
                wrapped_read(file->fs->device, ADDR(block_2nd, (read_id / ptr_per_page) & (ptr_per_page - 1)), &block_3rd, sizeof(u32));
                wrapped_read(file->fs->device, ADDR(block_3rd, read_id & (ptr_per_page - 1)), &temp_block, sizeof(u32));
                free_block(file->fs, temp_block);
                if (read_id % ptr_per_page == 0)
                    free_block(file->fs, block_3rd);

                if (read_id % (ptr_per_page * ptr_per_page) == 0)
                    free_block(file->fs, block_2nd);
                
                if (read_id == 0)
                    free_block(file->fs, inode.triple_ind);
            }
        }
    }
    inode.size_bytes = newsize;
    fs_file_sync(file, &inode);
    return 0;
}

u32 fs_file_get_permission(const void *self) {
    Inode inode;
    get_inode((const FSFile *)self, &inode);
    return inode.permission;
}

void fs_file_set_permission(void *self, u32 permission) {
    Inode inode;
    get_inode((const FSFile *)self, &inode);
    inode.permission = permission;
    fs_file_sync((FSFile *)self, &inode);
}

u64 fs_file_get_size(const void *self) {
    Inode inode;
    get_inode((const FSFile *)self, &inode);
    return inode.size_bytes;
}

void fs_file_set_size(void *self, u64 size) {
    Inode inode;
    get_inode((const FSFile *)self, &inode);
    inode.size_bytes = size;
    fs_file_sync((FSFile *)self, &inode);
}

void wrap_fsfile(File *out, FSFile *in) {
    out->super = in;
    Inode inode;
    get_inode(in, &inode);
    out->get_permission = fs_file_get_permission;
    out->set_permission = fs_file_set_permission;
    out->get_size = fs_file_get_size;
    out->set_size = fs_file_set_size;
    out->type = inode.type;

    out->read = (i64 (*)(void *, u64, void *, u64)) fs_file_read;
    out->write = (i64 (*)(void *, u64, const void *, u64)) fs_file_write;
}