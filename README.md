
# EndrOS

> [!WARNING]
> This OS is unfinished. It is a demonstration of OS for me to learn and is recommended for OS course projects, learning OS and RISC-V, etc.
> You are not encouraged to use this in production.

<br>
<br>

Endr**OS** - Minimal OS with Linux API/ABI compatibility for RISC-V

---

This project is a demonstration of the basic components of an OS kernel.
The project is inspired by [rCoreOS](https://rcore-os.cn), but it is built completely without third-party libraries and only C.

You are free to use, learn, and redistribute under GNU GPL v3 license.

### Features

The system features, ordered in Three Easy Pieces:

- [x] **Virtualization**:
    - [x] Page Table
    - [x] Recursive Page Table for kernel space mapping
    - [x] Isolated Processes
    - [x] Round-Robin Scheduler
    - [ ] Users / Groups
    - [ ] Dynamic Linker
    - [ ] MLFQ
- [ ] **Concurrency** (Not implemented yet)
    - [ ] Multi-core support
    - [ ] Threads
    - [ ] Locks
    - [ ] Asynchronous I/O
- [x] **Persistence**
    - [x] Read-only Filesystem (VSFS)
    - [x] VirtIO Block Device Driver
    - [x] Buffered Block
    - [x] Files
    - [x] Everything is a File
    - [ ] VirtIO Graphics Driver
    - [ ] DTB Parsing (now only has a function to print)

For user programs, it has:

- `init`
- `sh`
- `ls`


### Build

Make sure you have [`riscv-gnu-toolchain`](https://github.com/riscv-collab/riscv-gnu-toolchain) installed.

```bash
$ git clone https://github.com/endaytrer/endros.git
$ cd endros
$ make defconfig
$ make
```

The OS Kernel (ELF) is `endros`, and the raw binary is `endros.img`.

The process also builds a disk image for user programs: `fs.img`. It is done by `script/mkfs.py`, which you can use to create any VSFS image from any directory by using:

```bash
$ script/mkfs.py -o fs.img DIRECTORY
```

### Run

Currently, EndrOS only supports to be run in [QEMU](https://www.qemu.org). Make sure you have `qemu-system-riscv64`.

```bash
$ make qemu
```
