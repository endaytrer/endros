#!/usr/bin/env python

import os
import mmap
import argparse
import re
import math
import ctypes
from typing import NamedTuple
BLOCK_SIZE = 4096
DEFAULT_INODES = 4096
DEFAULT_BLOCKS = 1024
MAGIC = 0x0a050f05
DIRECT_PTRS = 25

VSFS_DIRECTORY = 0
VSFS_FILE = 1
VSFS_SLINK = 2


class Super(ctypes.Structure):
    _fields_ = [('magic', ctypes.c_uint32),
                ('inode_bitmap_blocks', ctypes.c_uint32), # length (in blocks) of inode bitmap
                ('data_bitmap_blocks', ctypes.c_uint32), # length (in blocks) of data bitmap
                ('inode_table_blocks', ctypes.c_uint32) # length (in blocks) of inode_table
    ]

INODE_SIZE = 128

class Inode(ctypes.Structure):
    _fields_ = [('type', ctypes.c_uint32),
                ('privilege', ctypes.c_uint32),
                ('size_bytes', ctypes.c_uint64),
                ('direct', ctypes.c_uint32 * DIRECT_PTRS),
                ('single_ind', ctypes.c_uint32),
                ('double_ind', ctypes.c_uint32),
                ('triple_ind', ctypes.c_uint32)
    ]


DIR_ENTRY_SIZE = 64

class DirEntry(ctypes.Structure):
    _fields_ = [('name', ctypes.c_char * 60),
                ('inode', ctypes.c_uint32)]

parser = argparse.ArgumentParser(prog='mkfs.py', description='Making vsfs raw disk image from directory')
parser.add_argument('directory')
parser.add_argument('-o', '--output', required=True, help='Output disk image file')
parser.add_argument('-s', '--size', default=str(DEFAULT_BLOCKS * BLOCK_SIZE), help=f"Number of blocks, default {DEFAULT_BLOCKS}")
parser.add_argument('-i', '--inodes', default=str(DEFAULT_INODES), help=f"Number of blocks, default {DEFAULT_INODES}")

# constants after argument parsed

mm: mmap.mmap

current_block = 0

inode_bitmap_ptr: int
inode_bitmap_size_bytes: int

data_bitmap_ptr: int
data_bitmap_size_bytes: int

inode_table_ptr: int
inode_table_size_bytes: int

data_ptr: int
data_size: int


def parse_size(size: str) -> int:
    """
    return the number of blocks given by size: *, *k/K/kiB/KiB, *M/MiB, *G/GiB
    """
    size_regex = r'^\d+(([kKMG])(iB)?)?$'

    size_match = re.match(size_regex, size)

    if size_match is None:
        raise Exception('Invalid disk size')
    
    match_groups = size_match.groups() # example: ('kiB', 'k', 'iB'), ('k', 'k', None), (None, None, None)
    
    multiplier = 1

    if match_groups[1] == 'k' or match_groups[1] == 'K':
        multiplier = 1024
    
    if match_groups[1] == 'M':
        multiplier = 1024 * 1024

    if match_groups[1] == 'G':
        multiplier = 1024 * 1024 * 1024

    if match_groups[0]:
        size = size[:-len(match_groups[0])]

    size_num = int(size) * multiplier
    return size_num


# util functions

def bitmap_locate(number: int) -> (int, int):
    return (number // 8, number & 0x7)

def filesize_to_blocks(size: int) -> list[list[int]]:
    """
    size: size in bytes:
    return:
        [[direct], [1, direct], [1, second, direct], [1, third, second, direct]]
    """
    total_blocks = int(math.ceil(size / BLOCK_SIZE))
    # direct
    size_max = DIRECT_PTRS
    if (total_blocks <= size_max):
        return [[int(math.ceil(size / BLOCK_SIZE))]]
    
    # one level indirection
    size_max = (DIRECT_PTRS + (BLOCK_SIZE // 4))
    if (total_blocks <= size_max):
        covered_blocks = DIRECT_PTRS
        uncovered_blocks = total_blocks - covered_blocks
        return [[DIRECT_PTRS], [1, uncovered_blocks]]
    
    # two level indirection
    size_max = (DIRECT_PTRS + (BLOCK_SIZE // 4) + (BLOCK_SIZE // 4) * (BLOCK_SIZE // 4))
    if (total_blocks <= size_max):
        covered_blocks = DIRECT_PTRS + BLOCK_SIZE // 4
        uncovered_blocks = total_blocks - covered_blocks
        second_level_blocks = int(math.ceil(uncovered_blocks / (BLOCK_SIZE // 4)))
        return [[DIRECT_PTRS], [1, BLOCK_SIZE // 4], [1, second_level_blocks, uncovered_blocks]]
    
    # three level indirection
    size_max = (DIRECT_PTRS + (BLOCK_SIZE // 4) + (BLOCK_SIZE // 4) * (BLOCK_SIZE // 4) + (BLOCK_SIZE // 4)  * (BLOCK_SIZE // 4) * (BLOCK_SIZE // 4))
    if (total_blocks <= size_max):
        covered_blocks = DIRECT_PTRS + BLOCK_SIZE // 4 + (BLOCK_SIZE // 4) * (BLOCK_SIZE // 4)
        uncovered_blocks = total_blocks - covered_blocks
        second_level_blocks = int(math.ceil(uncovered_blocks / (BLOCK_SIZE // 4)))
        third_level_blocks = int(math.ceil(second_level_blocks / (BLOCK_SIZE // 4)))
        return [[DIRECT_PTRS], [1, BLOCK_SIZE // 4], [1, BLOCK_SIZE // 4, (BLOCK_SIZE // 4)  * (BLOCK_SIZE // 4)], [1, third_level_blocks, second_level_blocks, uncovered_blocks]]


# filesystem dependent functions

def allocate_contiguous_blocks(from_id: int, size: int):
    global mm
    global inode_bitmap_blocks
    global data_bitmap_blocks
    global inode_table_blocks
    global inode_bitmap_ptr
    global inode_bitmap_size_bytes
    global data_bitmap_ptr
    global data_bitmap_size_bytes
    global inode_table_ptr
    global inode_table_size_bytes
    global data_ptr
    global data_size
    global current_block

    (ptr_from, offset_from) = bitmap_locate(from_id)
    (ptr_to, offset_to) = bitmap_locate(from_id + size - 1)

    if ptr_to >= data_bitmap_size_bytes:
        raise Exception('Unable to allocate, disk is full')

    if ptr_from == ptr_to:
        mm.seek(data_bitmap_ptr + ptr_from)
        original_byte = mm.read_byte()
        for i in range(offset_from, offset_to + 1):
            original_byte |= (1 << i)

        mm.seek(data_bitmap_ptr + ptr_from)
        mm.write_byte(original_byte)
        return
    
    mm.seek(data_bitmap_ptr + ptr_from)
    original_byte = mm.read_byte()
    original_byte |= (0xff << offset_from) & 0xff
    mm.seek(data_bitmap_ptr + ptr_from)
    mm.write_byte(original_byte)

    for _ in range(ptr_from + 1, ptr_to):
        mm.write_byte(0xff)
    
    original_byte = mm.read_byte()
    original_byte |= 0xff >> (0x7 - offset_to)
    mm.seek(data_bitmap_ptr + ptr_to)
    mm.write_byte(original_byte)

def allocate_file_blocks(inode: Inode, size_bytes: int, data: bytearray) -> None:

    global mm
    global inode_bitmap_blocks
    global data_bitmap_blocks
    global inode_table_blocks
    global inode_bitmap_ptr
    global inode_bitmap_size_bytes
    global data_bitmap_ptr
    global data_bitmap_size_bytes
    global inode_table_ptr
    global inode_table_size_bytes
    global data_ptr
    global data_size
    global current_block

    blocks = filesize_to_blocks(size_bytes)
    level = len(blocks) - 1
    num_blocks = sum([sum(i) for i in blocks])
    allocate_contiguous_blocks(current_block, num_blocks)
    for i in range(blocks[0][0]):
        inode.direct[i] = current_block + i
        mm.seek((current_block + i) * BLOCK_SIZE)
        mm.write(data[i * BLOCK_SIZE: (i + 1) * BLOCK_SIZE])
    
    current_block += sum(blocks[0])
    if level < 1:
        return

    # write indirect
    inode.single_ind = current_block
    single_ind_ptr = inode.single_ind * BLOCK_SIZE
    for i in range(blocks[1][1]):
        block = current_block + sum(blocks[1][:1]) + i
        index = sum([sum(j) for j in blocks[:1]]) + sum(blocks[1][:1]) + i
        mm.seek(single_ind_ptr + i * 4)
        mm.write(block.to_bytes(4, 'little'))

        mm.seek(block * BLOCK_SIZE)
        mm.write(data[index * BLOCK_SIZE: (index + 1) * BLOCK_SIZE])
        # write data

    current_block += sum(blocks[1])
    if level < 2:
        return
    # write ptr to 2-indirect
    inode.double_ind = current_block
    double_ind_ptr = inode.double_ind * BLOCK_SIZE
    mm.seek(double_ind_ptr)
    for i in range(blocks[2][1]):
        mm.write((current_block + sum(blocks[2][:1]) + i).to_bytes(4, 'little'))

    double_ind_second = current_block + sum(blocks[2][:1])
    double_ind_second_ptr = double_ind_second * BLOCK_SIZE
    for i in range(blocks[2][2]):
        block = current_block + sum(blocks[2][:2]) + i
        index = sum([sum(j) for j in blocks[:2]]) + sum(blocks[2][:2]) + i
        mm.seek(double_ind_second_ptr + i * 4)
        mm.write(block.to_bytes(4, 'little'))

        mm.seek(block * BLOCK_SIZE)
        mm.write(data[index * BLOCK_SIZE: (index + 1) * BLOCK_SIZE])


    current_block += sum(blocks[2])
    if level < 3:
        return
    
    inode.triple_ind = current_block
    triple_ind_ptr = inode.triple_ind * BLOCK_SIZE
    mm.seek(triple_ind_ptr)
    for i in range(blocks[3][1]):
        mm.write((current_block + sum(blocks[3][:1]) + i).to_bytes(4, 'little'))

    triple_ind_second = current_block + sum(blocks[3][:1])
    triple_ind_second_ptr = triple_ind_second * BLOCK_SIZE
    mm.seek(triple_ind_second_ptr)
    for i in range(blocks[3][2]):
        mm.write((current_block + sum(blocks[3][:2]) + i).to_bytes(4, 'little'))

    triple_ind_third = current_block + sum(blocks[3][:2])
    triple_ind_third_ptr = triple_ind_third * BLOCK_SIZE
    for i in range(blocks[3][3]):
        block = current_block + sum(blocks[3][:3]) + i
        index = sum([sum(j) for j in blocks[:3]]) + sum(blocks[3][:3]) + i
        mm.seek(triple_ind_third_ptr + i * 4)
        mm.write((current_block + sum(blocks[3][:3]) + i).to_bytes(4, 'little'))

        mm.seek(block * BLOCK_SIZE)
        mm.write(data[index * BLOCK_SIZE: (index + 1) * BLOCK_SIZE])

def create_file(inode: int, type: int, privilege: int, size_bytes: int, data: bytearray) -> None:

    global mm
    global inode_bitmap_blocks
    global data_bitmap_blocks
    global inode_table_blocks
    global inode_bitmap_ptr
    global inode_bitmap_size_bytes
    global data_bitmap_ptr
    global data_bitmap_size_bytes
    global inode_table_ptr
    global inode_table_size_bytes
    global data_ptr
    global data_size
    global current_block

    # mark inode bitmap to 1
    (ptr, offset) = bitmap_locate(inode)

    if ptr >= inode_bitmap_size_bytes:
        raise Exception('Unable to allocate excessive inode')
    mm.seek(inode_bitmap_ptr + ptr)
    original_byte = int(mm.read_byte())
    mm.seek(inode_bitmap_ptr + ptr)
    mm.write_byte(original_byte | (1 << offset))

    # set inode info
    inode_obj = Inode(type, privilege, size_bytes)
    allocate_file_blocks(inode_obj, size_bytes, data)
    mm.seek(inode_table_ptr + inode * INODE_SIZE)
    mm.write(inode_obj)

def create_dir_bfs(path: str, inode: int) -> None:
    global mm
    global inode_bitmap_blocks
    global data_bitmap_blocks
    global inode_table_blocks
    global inode_bitmap_ptr
    global inode_bitmap_size_bytes
    global data_bitmap_ptr
    global data_bitmap_size_bytes
    global inode_table_ptr
    global inode_table_size_bytes
    global data_ptr
    global data_size
    global current_block

    bfs_queue = [(path, inode, inode)]

    current_inode = inode + 1

    while len(bfs_queue) != 0:
        path, inode, p_inode = bfs_queue.pop(0)

        if not os.path.isdir(path):
            # treat symbolic links as regular files
            stats = os.stat(path)
            type = VSFS_FILE
            privilege = stats.st_mode
            size = stats.st_size
            
            with open(path, 'rb') as f:
                data = f.read(size)
            
            create_file(inode, type, privilege, size, data)
            continue

        stats = os.stat(path)
        type = VSFS_DIRECTORY
        privilege = stats.st_mode
        subdir = [(x, current_inode + i) for i, x in enumerate(os.listdir(path))]

        size = (2 + len(subdir)) * DIR_ENTRY_SIZE
            
        data = bytearray(DirEntry(".".encode(), inode))
        data += DirEntry("..".encode(), p_inode)
            
        for name, new_inode in subdir:
            bfs_queue.append((path + '/' + name, new_inode, inode))
            data += DirEntry(name.encode(), new_inode)

        create_file(inode, type, privilege, size, data)
        current_inode += len(subdir)



            





def main() -> None:
    global mm
    global inode_bitmap_blocks
    global data_bitmap_blocks
    global inode_table_blocks
    global inode_bitmap_ptr
    global inode_bitmap_size_bytes
    global data_bitmap_ptr
    global data_bitmap_size_bytes
    global inode_table_ptr
    global inode_table_size_bytes
    global data_ptr
    global data_size
    global current_block

    args = parser.parse_args()

    size = parse_size(args.size)

    if not os.path.isdir(args.directory):
        raise Exception(f"{args.directory} is not a directory")

    fd = os.open(args.output, os.O_RDWR | os.O_CREAT | os.O_TRUNC)
    if fd < 0:
        raise Exception(f"cannot open file {args.output}")
    os.ftruncate(fd, size)
    mm = mmap.mmap(fd, 0)
    total_blocks = int(math.ceil(size / BLOCK_SIZE))
    total_inodes = int(args.inodes)

    inode_bitmap_blocks = int(math.ceil(int(math.ceil(total_inodes / 8))) / BLOCK_SIZE)
    data_bitmap_blocks = int(math.ceil(int(math.ceil(total_blocks / 8))) / BLOCK_SIZE)
    inode_table_blocks = int(math.ceil((total_inodes * INODE_SIZE)) / BLOCK_SIZE)


    inode_bitmap_ptr = BLOCK_SIZE
    inode_bitmap_size_bytes = int(math.ceil(total_inodes / 8))

    data_bitmap_ptr = (1 + inode_bitmap_blocks) * BLOCK_SIZE
    data_bitmap_size_bytes = int(math.ceil(total_blocks / 8))

    inode_table_ptr = (1 + inode_bitmap_blocks + data_bitmap_blocks) * BLOCK_SIZE
    inode_table_size_bytes = total_inodes * INODE_SIZE

    data_ptr = (1 + inode_bitmap_blocks + data_bitmap_blocks + inode_table_blocks) * BLOCK_SIZE
    data_size = size - data_ptr

    super_block = Super(MAGIC, inode_bitmap_blocks, data_bitmap_blocks)

    mm.seek(0)
    mm.write(super_block)

    current_block = 1 + inode_bitmap_blocks + data_bitmap_blocks + inode_table_blocks

    allocate_contiguous_blocks(0, current_block)

    create_dir_bfs(args.directory, 2)

    # saving file
    mm.flush()
    mm.close()
    os.close(fd)


if __name__ == "__main__":
    main()
