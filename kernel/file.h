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
    void *super;

    /// the type of a file immutable.
    /// If it is a file / directory / symbolic link, it is an FSFile.
    /// Otherwise, it is a virtual file presented in memory
    FileType type;

    /// for mutable variables, use getter / setters for consistency.
    u32 (*get_permission)(const void *self);
    void (*set_permission)(void *self, u32 permission);
    u64 (*get_size)(const void *self);
    void (*set_size)(void *self, u64 size);
    i64 (*read)(void *self, u64 offset, void *buf, u64 size);
    i64 (*write)(void *self, u64 offset, const void *buf, u64 size);
} File;

typedef struct {
    bool occupied;
    int open_flags;
    File file;
    u64 seek;
} FileDescriptor;

i64 wrapped_read(File *file, u64 offset, void *buf, u64 size);
i64 wrapped_write(File *file, u64 offset, const void *buf, u64 size);

i64 trunc_file(File *file, u64 size);

// static permissions
u32 static_r(const void *);
u32 static_w(const void *);
u32 static_rw(const void *);

#endif