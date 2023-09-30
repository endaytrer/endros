#include "file.h"
#include "mem.h"
#include "sbi.h"

i64 stdin_read(void *self, u64 offset, void *buf, u64 size) {
    if (offset != 0 || size != 1) {
        return -1;
    }
    *(char *)buf = kgetc();
    return 0;
}

i64 stdout_write(void *self, u64 offset, const void *buf, u64 size) {
    if (offset != 0) {
        return -1;
    }
    for (u64 i = 0; i < size; i++) {
        if (((char *)buf)[i] == '\0') return 0;
        kputc(((char *)buf)[i]);
    }
    return 0;
}

File stdin = {
    .permission = PERMISSION_R,
    .type = DEVICE,
    .super = NULL,
    .read = stdin_read,
    .write = NULL,
    .size = ~((u64)0),
};

File stdout = {
    .permission = PERMISSION_W,
    .type = DEVICE,
    .super = NULL,
    .read = NULL,
    .write = stdout_write,
    .size = ~((u64)0),
};

File stderr = {
    .permission = PERMISSION_W,
    .type = DEVICE,
    .read = NULL,
    .write = stdout_write,
    .size = ~((u64)0),
};