#ifndef _K_FS_FILE_H
#define _K_FS_FILE_H
#include "filesystem.h"
#include "file.h"



/// @brief Read the file metadata (type, size, permission) from disk to memory
/// @param file INOUT, the file to read. Must specify inum and fs.
/// @return status
i64 get_inode(const FSFile *file, Inode *out_inode);

/// @brief Write file metadata (type, size, permission) to disk.
/// @param file the file to sync.
/// @return status
i64 fs_file_sync(const FSFile *file, const Inode *inode);

i64 fs_file_read(FSFile *file, u64 offset, void *buf, u64 size);
i64 fs_file_write(FSFile *file, u64 offset, const void *buf, u64 size);
i64 fs_file_truncate(FSFile *file, u64 newsize);

void wrap_fsfile(File *out, FSFile *in);
  
#endif