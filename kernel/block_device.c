#include <string.h>
#include "file.h"
#include "block_device.h"
#include "pagetable.h"
#include "printk.h"

BlockBuffer *get_block_buffer(BufferedBlockDevice *buffered_blk_dev, u64 block_id) {
    // find in cache
    BlockBuffer *ptr = buffered_blk_dev->buffer_head;
    BlockBuffer *prev = NULL;
    while (ptr) {
        if (ptr->block_id == block_id) {
            // cache hit
            // make it to first of cache.
            if (prev) { // if it IS head, do not do anything
                prev->next = ptr->next;
                ptr->next = buffered_blk_dev->buffer_head;
                buffered_blk_dev->buffer_head = ptr;
            }
            return buffered_blk_dev->buffer_head;
        }
        if (ptr->next == NULL) break;
        prev = ptr;
        ptr = ptr->next;
    }
    // cache miss, add to cache;
    int cache_size = buffered_blk_dev->cache_size;
    if (cache_size < CACHE_BLOCKS) {
        vpn_t blk_vpn;
        // not zeroing because it is set not valid.
        pfn_t blk_pfn = dmalloc(&blk_vpn, 1, false);

        // if no blocks are ever evicted, the last free index is always num_blocks
        buffered_blk_dev->buffer_list[cache_size] = (BlockBuffer) {
            .valid = false,
            .dirty = false,
            .block_id = block_id,
            .next = buffered_blk_dev->buffer_head,
            .vpn = blk_vpn,
            .pfn = blk_pfn
        };
        buffered_blk_dev->buffer_head = buffered_blk_dev->buffer_list + cache_size;
        buffered_blk_dev->cache_size++;
        return buffered_blk_dev->buffer_head;
    }
    // evict the last page;
    // is stored in ptr, so just read to it;
    // if the page is dirty, write back to disk

    if (ptr->valid && ptr->dirty) {
        buffered_blk_dev->write_block(buffered_blk_dev->super, ptr->block_id * (PAGESIZE / SECTOR_SIZE), ptr->vpn, ptr->pfn);
    }
    ptr->next = buffered_blk_dev->buffer_head;
    prev->next = NULL;
    buffered_blk_dev->buffer_head = ptr;
    return buffered_blk_dev->buffer_head;
}
void read_buffered_block(BufferedBlockDevice *buffered_blk_dev, u64 block_id, void *buf) {
    BlockBuffer *ptr = get_block_buffer(buffered_blk_dev, block_id);
    if (!ptr->valid) {
        buffered_blk_dev->read_block(buffered_blk_dev->super, block_id * (PAGESIZE / SECTOR_SIZE), ptr->vpn, ptr->pfn);
        ptr->valid = true;
    }
    void *src = (void *)PAGE_2_ADDR(ptr->vpn);
    memcpy(buf, src, PAGESIZE);
}

void write_buffered_block(BufferedBlockDevice *buffered_blk_dev, u64 block_id, const void *buf) {
    BlockBuffer *ptr = get_block_buffer(buffered_blk_dev, block_id);
    ptr->valid = true;
    ptr->dirty = true;
    void *dst = (void *)PAGE_2_ADDR(ptr->vpn);
    memcpy(dst, buf, PAGESIZE);
}

void translate_bytes(BufferedBlockDevice *buffered_blk_dev, u64 offset, void *buffer, u64 size, bool write) {
    u64 page_offset = OFFSET(offset);
    u64 page_max = page_offset + size;
    for (u64 block_id = ADDR_2_PAGE(offset); block_id < ADDR_2_PAGEUP(offset + size); ++block_id) {
        u64 temp_page;
        if (page_max > PAGESIZE) {
            temp_page = PAGESIZE;
            page_max -= PAGESIZE;
        } else {
            temp_page = page_max;
        }
        BlockBuffer *ptr = get_block_buffer(buffered_blk_dev, block_id);
        if (write) {

            if (!ptr->valid && (page_offset != 0 || temp_page != PAGESIZE)) {
                if (buffered_blk_dev->read_block(buffered_blk_dev->super, block_id * (PAGESIZE / SECTOR_SIZE), ptr->vpn, ptr->pfn) != 0)
                    printk("[kernel] read block failed\n");
                else
                    ptr->valid = true;
            }
            ptr->dirty = true;
            memcpy((void *)ADDR(ptr->vpn, page_offset), buffer, temp_page - page_offset);
        } else {
            if (!ptr->valid) {
                if (buffered_blk_dev->read_block(buffered_blk_dev->super, block_id * (PAGESIZE / SECTOR_SIZE), ptr->vpn, ptr->pfn) != 0)
                    printk("[kernel] read block failed\n");
                else
                    ptr->valid = true;
            }
            memcpy(buffer, (void *)ADDR(ptr->vpn, page_offset), temp_page - page_offset);
        }
        buffer = (void *)((u64)buffer +  temp_page - page_offset);
        page_offset = 0;
    }

}
i64 read_bytes(BufferedBlockDevice *buffered_blk_dev, u64 offset, void *buffer, u64 size) {
    translate_bytes(buffered_blk_dev, offset, buffer, size, false);
    return 0;
}

i64 write_bytes(BufferedBlockDevice *buffered_blk_dev, u64 offset, const void *buffer, u64 size) {
    translate_bytes(buffered_blk_dev, offset, (void *)buffer, size, true);
    return 0;
}
u64 get_blk_size(const void *self) {
    return ((const BufferedBlockDevice *)self)->size;
}

void wrap_block_buffer_file(File *out, BufferedBlockDevice *in) {
    out->super = in;
    out->type = DEVICE;
    out->get_permission = static_rw;
    out->set_permission = NULL;
    out->get_size = get_blk_size;
    out->set_size = NULL; // cannot set device size;
    out->read = (i64 (*)(void *, u64, void *, u64)) read_bytes;
    out->write = (i64 (*)(void *, u64, const void *, u64)) write_bytes;
}