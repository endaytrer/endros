#ifndef _K_BLOCK_DEVICE_H
#define _K_BLOCK_DEVICE_H
#include <type.h>
#include "spinlock.h"

#define BLOCK_SIZE 4096
typedef struct buf {
    i32 valid;
    i32 disk;
    u32 dev;
    u32 block_num;
    spinlock_t lock;
    u32 refcount;
    struct buf *prev;
    struct buf *next;
    u8 data[BLOCK_SIZE];
} BlockBuffer;
#endif