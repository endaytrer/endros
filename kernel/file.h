#ifndef _K_FILE
#define _K_FILE
#include <type.h>

#define PERMISSION_R 4
#define PERMISSION_W 2
#define PERMISSION_X 1

typedef struct {
    enum {
        DIRECTORY,
        FILE,
        SYMBOLIC_LINK,
        DEVICE
    } type;
    int permission;
    u64 size;
    i64 (*read)(u64 offset, char *buf, u64 size);
    i64 (*write)(u64 offset, const char *buf, u64 size);
} File;

#endif