#include "syscall.h"
#include "printk.h"
#include "fcntl.h"
#include "mem.h"
#include "process.h"
#include "sbi.h"
#include "filesystem.h"
#include <string.h>
/**
 * if write is set, from user buf to kernel buf
 * else, from kernel buf to user buf
*/
void translate_buffer(PTReference_2 *ptref_base, void *kernel_buf, void *user_buf, u64 size, bool write) {
    u64 page_offset = OFFSET(user_buf);
    u64 page_max = page_offset + size;
    for (vpn_t user_vpn = ADDR_2_PAGE(user_buf); user_vpn != ADDR_2_PAGEUP(user_buf + size); ++user_vpn) {
        u64 temp_page;
        if (page_max > PAGESIZE) {
            temp_page = PAGESIZE;
            page_max -= PAGESIZE;
        } else {
            temp_page = page_max;
        }
        vpn_t kernel_vpn = walkupt(ptref_base, user_vpn);
        if (write)
            memcpy(kernel_buf, (void *)ADDR(kernel_vpn, page_offset), temp_page - page_offset);
        else
            memcpy((void *)ADDR(kernel_vpn, page_offset), kernel_buf, temp_page - page_offset);
        kernel_buf = (void *)((u64)kernel_buf +  temp_page - page_offset);
        page_offset = 0;
    }
}
i64 sys_read(u64 fd, char *buf, u64 size) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    if (proc->opened_files[fd].file == NULL) {
        return -1;
    }
    FileDescriptor *file_desc = proc->opened_files + fd;
    if (O_ACCMODE(file_desc->open_flags) != O_RDWR && O_ACCMODE(file_desc->open_flags) != O_RDONLY) {
        printk("[kernel] process don't have read permission\n");
        return -1;
    }
    char *kernel_ptr = kalloc(size);

    // char *kernel_ptr = (char *)ADDR(kernel_vpn, OFFSET(buf));

    wrapped_read(file_desc->file, file_desc->seek, kernel_ptr, size);

    // translate kernel buffer to user
    translate_buffer(proc->ptref_base, kernel_ptr, buf, size, false);

    kfree(kernel_ptr, size);
    return 0;
}

i64 sys_write(u64 fd, const char *buf, u64 size) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    if (proc->opened_files[fd].file == NULL) {
        return -1;
    }
    FileDescriptor *file_desc = proc->opened_files + fd;
    if (O_ACCMODE(file_desc->open_flags) != O_RDWR && O_ACCMODE(file_desc->open_flags) != O_WRONLY) {
        printk("[kernel] process don't have write permission\n");
        return -1;
    }
    if ( file_desc->file->size < file_desc->seek + size) {
        if (file_desc->open_flags & O_TRUNC)
            trunc_file(file_desc->file, size);
        else {
            printk("[kernel] write to larger filesize without permission to truncate\n");
            return -1;
        }
    }
    char *kernel_ptr = kalloc(size);
    translate_buffer(proc->ptref_base, kernel_ptr, (void *)buf, size, true);

    // char *kernel_ptr = (char *)ADDR(kernel_vpn, OFFSET(buf));

    wrapped_write(file_desc->file, file_desc->seek, kernel_ptr, size);

    // translate kernel buffer to user

    kfree(kernel_ptr, size);
    return 0;
}