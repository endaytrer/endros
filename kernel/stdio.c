#include "file.h"
#include "mem.h"
#include "sbi.h"

i64 stdin_read(u64 offset, char *buf, u64 size) {
    if (offset != 0 || size != 1) {
        return -1;
    }
    *buf = kgetc();
    return 0;
}

i64 stdout_write(u64 offset, const char *buf, u64 size) {
    if (offset != 0) {
        return -1;
    }
    for (u64 i = 0; i < size; i++) {
        if (buf[i] == '\0') return 0;
        kputc(buf[i]);
    }
    return 0;
}

File stdin = {
    .permission = PERMISSION_R,
    .type = DEVICE,
    .read = stdin_read,
    .write = NULL,
    .size = 1,
};

File stdout = {
    .permission = PERMISSION_W,
    .type = DEVICE,
    .read = NULL,
    .write = stdout_write,
    .size = 0
};

File stderr = {
    .permission = PERMISSION_W,
    .type = DEVICE,
    .read = NULL,
    .write = stdout_write,
    .size = 0
};