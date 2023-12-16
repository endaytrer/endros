#include "file.h"
#include "printk.h"
#include "fs_file.h"

i64 wrapped_read(File *file, u64 offset, void *buf, u64 size) {
    if (!(file->get_permission(file->super) & PERMISSION_R)) {   
        printk("[kernel] read on unreadable file\n");
        return -1;
    }
    if (offset + size > file->get_size(file->super)) {
        return -1;
    }
    return file->read(file->super, offset, buf, size);
}

i64 wrapped_write(File *file, u64 offset, const void *buf, u64 size) {
    if (!(file->get_permission(file->super) & PERMISSION_W)) {   
        printk("[kernel] write on unwritable file\n");
        return -1;
    }
    if (offset + size > file->get_size(file->super)) {
        printk("[kernel] write to large\n");
        return -1;
    }
    return file->write(file->super, offset, buf, size);
}

i64 trunc_file(File *file, u64 size) {
    if (file->type != DEVICE)
        return fs_file_truncate(file->super, size); // is a fs_file

    return -1;
}

u32 static_r(const void *_) {
    return PERMISSION_R;
}

u32 static_w(const void *_) {
    return PERMISSION_W;
}

u32 static_rw(const void *_) {
    return PERMISSION_R | PERMISSION_W;
}
