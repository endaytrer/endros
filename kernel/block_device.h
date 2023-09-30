#ifndef _K_BLOCK_DEVICE_H
#define _K_BLOCK_DEVICE_H
#include <type.h>
#include "mem.h"

#define SECTOR_SIZE 512
#define CACHE_BLOCKS 64
// an lru cache of blocks.
typedef struct buf {
    bool valid;
    bool dirty;
    u64 block_id;
    vpn_t vpn;
    pfn_t pfn;
    struct buf *next;
} BlockBuffer;

typedef struct {
    BlockBuffer *buffer_head;
    int cache_size;
    BlockBuffer buffer_list[CACHE_BLOCKS];
    void *super;
    i64 (*read_block)(void *self, u64 sector, vpn_t vpn, pfn_t pfn);
    i64 (*write_block)(void *self, u64 sector, vpn_t vpn, pfn_t pfn);
} BufferedBlockDevice;


void init_buffered_block_device(BufferedBlockDevice *buffered_blk_dev, void *super,
    i64 (*read_block)(void *self, u64 sector, vpn_t vpn, pfn_t pfn),
    i64 (*write_block)(void *self, u64 sector, vpn_t vpn, pfn_t pfn)
);

BlockBuffer *get_block_buffer(BufferedBlockDevice *buffered_blk_dev, u64 block_id);

void read_buffered_block(BufferedBlockDevice *buffered_blk_dev, u64 block_id, void *buf);
void write_buffered_block(BufferedBlockDevice *buffered_blk_dev, u64 block_id, const void *buf);

i64 read_bytes(BufferedBlockDevice *buffered_blk_dev, u64 offset, void *buffer, u64 size);
i64 write_bytes(BufferedBlockDevice *buffered_blk_dev, u64 offset, const void *buffer, u64 size);
#endif