#ifndef _K_FILESYSTEM_H
#define _K_FILESYSTEM_H
#include "block_device.h"
#include "file.h"

#define BLOCK_SIZE          4096
#define DEFAULT_INODES      4096
#define DEFAULT_BLOCKS      1024
#define MAGIC               0x0a050f05
#define DIRECT_PTRS         25

#define BITMAP_OFFSET(x) ((x) >> 3)
#define BITMAP_BIT(x) ((x) & 0x7)
#define BITMAP_GET(byte, x) ((byte >> BITMAP_BIT(x)) & 1)

struct filesystem_t;

typedef struct fs_file_t {
    FileType type;
    struct filesystem_t *fs;
    u32 inum;
    int permission;
    u64 size;
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
    FileType type;
    u32 permission;
    u64 size_bytes;
    u32 direct[DIRECT_PTRS];
    u32 single_ind;
    u32 double_ind;
    u32 triple_ind;
} Inode;

typedef struct {
    char name[60];
    u32 inode;
} DirEntry;

typedef struct filesystem_t {
    File *device;
    SuperBlock super;
    struct fs_file_t root;

    /// Inode bitmap offset, represented all in blocks. Use `ADDR(page, offset)` to get corresponding block.
    u64 inode_bitmap_offset; 
    u64 data_bitmap_offset;
    u64 inode_table_offset;
} Filesystem;

extern Filesystem rootfs;
void init_filesystem(void);
void create_filesystem(Filesystem *filesystem, File *dev);
void ls(struct fs_file_t *dir);
i64 getfile(FSFile *cwd, const char *path, FSFile *fileout);
#endif