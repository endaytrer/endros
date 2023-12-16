#include "file.h"
#include "mem.h"
#include "sbi.h"

i64 stdin_read(void *self, u64 offset, void *buf, u64 size) {
    if (size != 1) {
        return -1;
    }
    *(char *)buf = kgetc();
    return 0;
}

i64 stdout_write(void *self, u64 offset, const void *buf, u64 size) {
    for (u64 i = 0; i < size; i++) {
        if (((char *)buf)[i] == '\0') return 0;
        kputc(((char *)buf)[i]);
    }
    return 0;
}
static const u64 std_size = ~((u64)0);


u64 static_size(const void *_) {
    return std_size;
}

File stdin = {
    .get_permission = static_r,
    .set_permission = NULL,
    .type = DEVICE,
    .super = NULL,
    .read = stdin_read,
    .write = NULL,
    .get_size = static_size,
    .set_size = NULL
};

File stdout = {
    .get_permission = static_w,
    .set_permission = NULL,
    .type = DEVICE,
    .super = NULL,
    .read = NULL,
    .write = stdout_write,
    .get_size = static_size,
    .set_size = NULL,
};

File stderr = {
    .get_permission = static_w,
    .set_permission = NULL,
    .type = DEVICE,
    .read = NULL,
    .write = stdout_write,
    .get_size = static_size,
    .set_size = NULL
};