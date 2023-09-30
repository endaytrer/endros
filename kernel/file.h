#ifndef _K_FILE
#define _K_FILE
#include <type.h>


#define PERMISSION_R 4
#define PERMISSION_W 2
#define PERMISSION_X 1
typedef enum {
    DIRECTORY,
    FILE,
    SYMBOLIC_LINK,
    DEVICE
} FileType;

typedef struct {
    /// the type of a file immutable.
    /// If it is a file / directory / symbolic link, it is an FSFile.
    /// Otherwise, it is a virtual file presented in memory
    FileType type; 
    void *super;
    int permission;
    u64 size;
    i64 (*read)(void *self, u64 offset, void *buf, u64 size);
    i64 (*write)(void *self, u64 offset, const void *buf, u64 size);
} File;

typedef struct {
    File *file;
    u64 seek;
    int open_flags;
} FileDescriptor;

i64 wrapped_read(File *file, u64 offset, void *buf, u64 size);
i64 wrapped_write(File *file, u64 offset, const void *buf, u64 size);

i64 trunc_file(File *file, u64 size);
#endif