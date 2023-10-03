#include <string.h>
#include <fcntl.h>
#include "syscall.h"
#include "printk.h"
#include "mem.h"
#include "process.h"
#include "filesystem.h"
#include "fs_file.h"
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
/// translate_2_pages
///
/// Is only used for translating paths from user space to kernel space.
/// Kernel buffer should be allocated by kalloc(2 * PAGESIZE)
/// \param kernel_buf should be allocated by `kalloc(2 * PAGESIZE)`
/// \param user_buf Null terminated string. Max length is one page.
void translate_2_pages(const PTReference_2 *ptref_base, void *kernel_buf, const void *user_buf) {
    u64 offset = OFFSET(user_buf);
    vpn_t user_vpn = ADDR_2_PAGE(user_buf);
    vpn_t kernel_vpn = walkupt(ptref_base, user_vpn);
    bool within_page = false;
    while (!within_page) {
        for (char *ptr = (char *)ADDR(kernel_vpn, offset); ptr != (char *)PAGE_2_ADDR(kernel_vpn + 1);) {
            *(char *)kernel_buf = *ptr;
            if (*ptr == '\0') {
                within_page = true;
                break;
            }
            kernel_buf = (void *)((u64)kernel_buf + 1);
            ptr++;
        }
        user_vpn += 1;
        kernel_vpn = walkupt(ptref_base, user_vpn);
        offset = 0;
    }
}

i64 sys_chdir(const char *filename) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    char *kernel_buf = kalloc(2 * PAGESIZE);
    translate_2_pages(proc->ptref_base, kernel_buf, filename);
    File file;
    i64 res = getfile(&proc->cwd_file, kernel_buf, &file);
    if (file.type != DIRECTORY) {
        return -1;
    }
    memcpy(&proc->cwd_file, file.super, sizeof(FSFile));
    kfree(file.super, sizeof(FSFile));
    kfree(kernel_buf, 2 * PAGESIZE);
    return res;
}
i64 sys_openat(i32 dfd, const char *filename, int flags, int mode) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    FSFile *cwd;
    if (proc->opened_files[dfd].file && proc->opened_files[dfd].file->type == DIRECTORY)
        cwd = proc->opened_files[dfd].file->super;
    else {
        cwd = &proc->cwd_file;
    }

    File *file = kalloc(sizeof(File));
    char *kernel_filename = kalloc(2 * PAGESIZE);
    translate_2_pages(proc->ptref_base, kernel_filename, filename);
    i64 res = getfile(cwd, kernel_filename, file);
    kfree(kernel_filename, 2 * PAGESIZE);
    if (res < 0) {
        return -1;
    }
    // find an empty slot for the fd
    int fd;
    for (fd = 0; fd < NUM_FILES; fd++) {
        if (proc->opened_files[fd].file)
            continue;
        proc->opened_files[fd].file = file;
        proc->opened_files[fd].open_flags = flags;
        proc->opened_files[fd].seek = 0;
        return fd;
    }
    return -1;
}

i64 sys_close(u32 fd) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    if (proc->opened_files[fd].file == NULL) {
        return -1;
    }
    if (proc->opened_files[fd].file->type != DEVICE) {
        kfree(proc->opened_files[fd].file->super, sizeof(FSFile));
        kfree(proc->opened_files[fd].file, sizeof(File));
    } else {
        kfree(proc->opened_files[fd].file, sizeof(File));
    }
    proc->opened_files[fd].file = NULL;
    proc->opened_files[fd].open_flags = 0;
    proc->opened_files[fd].seek = 0;
    return 0;
}

i64 sys_lseek(u32 fd, i64 off_high, i64 off_low, u64 *result, u32 whence) {
    // ignoring offset high, returning
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    if (off_high != 0) {
        printk("[kernel] offset high is not zero.\n");
        return -1;
    }
    if (whence == SEEK_SET)
        proc->opened_files[fd].seek = off_low;
    else if (whence == SEEK_CUR)
        proc->opened_files[fd].seek += off_low;
    else if (whence == SEEK_END)
        proc->opened_files[fd].seek = proc->opened_files[fd].file->size + off_low;
    else {
        printk("[kernel] Unsupported lseek whence\n");
        return -1;
    }
    vpn_t begin_vpn = ADDR_2_PAGE(result);
    u64 page_top = PAGE_2_ADDR(begin_vpn + 1);
    u64 page_1_sz = page_top - (u64)result;
    vpn_t kernel_vpn = walkupt(proc->ptref_base, begin_vpn);
    u64 *kernel_res = (u64 *)ADDR(kernel_vpn, OFFSET(result));
    if (page_1_sz >= sizeof(u64)) {
        *kernel_res = proc->opened_files[fd].seek;
    } else {
        memcpy(kernel_res, &proc->opened_files[fd].seek, page_1_sz);
        kernel_vpn = walkupt(proc->ptref_base, begin_vpn + 1);
        memcpy((void *)PAGE_2_ADDR(kernel_vpn), (void *)((u64)&proc->opened_files[fd].seek + page_1_sz), sizeof(u64) - page_1_sz);
    }
    return 0;
}

i64 sys_read(u32 fd, char *buf, u64 size) {
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
    int read_res;
    if ((read_res = wrapped_read(file_desc->file, file_desc->seek, kernel_ptr, size)) < 0) {
        return read_res;
    }

    // translate kernel buffer to user
    translate_buffer(proc->ptref_base, kernel_ptr, buf, size, false);


    kfree(kernel_ptr, size);

    // adding seek
    file_desc->seek += size;
    return 0;
}

i64 sys_write(u32 fd, const char *buf, u64 size) {
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
