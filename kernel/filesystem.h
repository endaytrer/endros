#ifndef _K_FILESYSTEM_H
#define _K_FILESYSTEM_H
#include "block_device.h"
#include "file.h"
#include "drivers/virtio.h"

#define BLOCK_SIZE          4096
#define DEFAULT_INODES      4096
#define DEFAULT_BLOCKS      1024
#define MAGIC               0x0a050f05
#define DIRECT_PTRS         25

#define BITMAP_OFFSET(x) ((x) >> 3)
#define BITMAP_BITS(x) ((x) & 0x7)
#define BITMAP_GET(byte, x) (((byte) >> BITMAP_BITS(x)) & 1)
#define BITMAP_SET(byte_ptr, x) *(byte_ptr) |= (1 << BITMAP_BITS(x))
#define BITMAP_CLEAR(byte_ptr, x) *(byte_ptr) &= ~(1 << BITMAP_BITS(x))
#define BITMAP_2_ADDR(offset, byte) (((offset) << 3) | BITMAP_BITS(byte))
struct filesystem_t;

typedef struct {
    FileType type;
    u32 permission;
    u64 size_bytes;
    u32 direct[DIRECT_PTRS];
    u32 single_ind;
    u32 double_ind;
    u32 triple_ind;
} Inode;

typedef struct fs_file_t {
    u32 inum;
    u32 rc; // reference count, for freeing after duplicating
    struct filesystem_t *fs;
    // Inode inode; // get inode whenever use to keep sync
} FSFile;

typedef struct {
    u32 magic;
    u32 inode_bitmap_blocks;
    u32 data_bitmap_blocks;
    u32 inode_table_blocks;
    u32 size_blocks;
    u32 root_inode;
} SuperBlock;


typedef struct {
    char name[60];
    u32 inode;
} DirEntry;

typedef struct filesystem_t {
    File *device;
    SuperBlock super;
    struct fs_file_t root;
    struct fs_file_t dev; // pointer to /dev
    /// Inode bitmap offset, represented all in blocks. Use `ADDR(page, offset)` to get corresponding block.
    u64 inode_bitmap_offset; 
    u64 data_bitmap_offset;
    u64 inode_table_offset;
} Filesystem;

extern Filesystem rootfs;
void init_filesystem(VirtIOHeader *blk_header);
void create_filesystem(Filesystem *filesystem, File *dev);
void sync_filesystem(void);
void ls(struct fs_file_t *dir);
i64 getfile(FSFile *cwd, const char *path, File *out, bool create);

i64 create_file(FSFile *parent, const char *filename, File *file);
i64 allocate_inode(Filesystem *fs);
i64 free_inode(Filesystem *fs, u32 inum);
i64 allocate_block(Filesystem *fs);
i64 free_block(Filesystem *fs, u32 block);
#endif