PREFIX := riscv64-linux-gnu-
CC := $(PREFIX)gcc
LD := $(PREFIX)ld
AS := $(PREFIX)as
AR := $(PREFIX)ar
GDB := gdb-multiarch
OBJCOPY := $(PREFIX)objcopy

QEMU := qemu-system-riscv64
QEMUOPTS := -machine virt -smp 4 -m 4G -nographic
ifdef DEBUG
QEMUOPTS += -s -S
endif

FSSIZE := 256MiB
INODES := 1024

CFLAGS := -ffreestanding -nostdlib -fno-omit-frame-pointer -g -O0

# Kernel
KERNEL := kernel
KERNEL_C_SRCS := $(wildcard $(KERNEL)/*.c)
KERNEL_C_HDRS := $(wildcard $(KERNEL)/*.h)
KERNEL_ASM_SRCS := $(wildcard $(KERNEL)/*.S)

KERNEL_OBJS := $(patsubst $(KERNEL)/%.c, $(KERNEL)/%.o, $(KERNEL_C_SRCS))
KERNEL_OBJS += $(patsubst $(KERNEL)/%.S, $(KERNEL)/%.o, $(KERNEL_ASM_SRCS))

KERNEL_ELF := coreos
KERNEL_BIN := coreos.img
KERNEL_LINKER_SCRIPT := kernel.ld
USER_LINKER_SCRIPT := user.ld

# User Library and Binary

USER := user
USER_LIB := $(USER)/libcoreos
USER_BIN := $(USER)/program

USER_LIB_C_SRCS := $(wildcard $(USER_LIB)/*.c)
USER_LIB_C_HDRS := $(wildcard $(USER_LIB)/*.h)
USER_LIB_ASM_SRCS := $(wildcard $(USER_LIB)/*.S)

USER_LIB_OBJS := $(patsubst $(USER_LIB)/%.c, $(USER_LIB)/%.o, $(USER_LIB_C_SRCS))
USER_LIB_OBJS += $(patsubst $(USER_LIB)/%.S, $(USER_LIB)/%.o, $(USER_LIB_ASM_SRCS))

USER_BIN_SRCS := $(wildcard $(USER_BIN)/*.c)
USER_BIN_OBJS := $(patsubst $(USER_BIN)/%.c, $(USER_BIN)/%.o, $(USER_BIN_SRCS))


# File System
FSIMG := fs.img
ROOTFS := rootfs
BIN_DST := $(ROOTFS)/bin
LIB_DST := $(ROOTFS)/lib
INCLUDE_DST := $(ROOTFS)/include

USER_ELFS := $(patsubst $(USER_BIN)/%.o, $(BIN_DST)/%, $(USER_BIN_OBJS))
USER_BINS := $(patsubst $(USER_BIN)/%.o, $(BIN_DST)/%.bin, $(USER_BIN_OBJS))
USER_LIB_STATIC := $(LIB_DST)/libcoreos.a
USER_LIB_DYNAMIC := $(LIB_DST)/libcoreos.so
USER_HDRS := $(patsubst $(USER_LIB)/%.h, $(INCLUDE_DST)/%.h, $(USER_LIB_C_HDRS))



# Compiling kernel

.PHONY: all clean qemu libcoreos kernel user

all: user kernel

kernel: $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) $^ -O binary $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -T $(KERNEL_LINKER_SCRIPT) -o $@ $(KERNEL_OBJS)

$(KERNEL)/%.o: $(KERNEL)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(KERNEL)/%.o: $(KERNEL)/%.S
	$(AS) $(ASFLAGS) -c -o $@ $^



# User library

libcoreos: $(USER_LIB_STATIC) $(USER_LIB_DYNAMIC)

$(USER_LIB_STATIC): $(USER_LIB_OBJS)
	$(AR) rcs $@ $^

$(USER_LIB_DYNAMIC): $(USER_LIB_OBJS)
	$(CC) -shared $(CFLAGS) -o $@ $^

$(USER_LIB)/%.o: $(USER_LIB)/%.c
	$(CC) -fPIC $(CFLAGS) -c -o $@ $^

$(USER_LIB)/%.o: $(USER_LIB)/%.S
	$(AS) -fPIC $(ASFLAGS) -c -o $@ $^

$(INCLUDE_DST)/%.h: $(USER_LIB)/%.h
	cp $^ $@



# User programs
user: $(FSIMG)

$(FSIMG): $(USER_BINS) $(USER_ELFS) $(USER_LIB_STATIC) $(USER_LIB_DYNAMIC) $(USER_HDRS) mkfs/mkfs
	mkfs/mkfs -o $@ $(ROOTFS) -s$(FSSIZE) -i$(INODES)

$(BIN_DST)/%.bin: $(BIN_DST)/%
	$(OBJCOPY) $^ --strip-all -O binary $@

$(BIN_DST)/%: $(USER_BIN)/%.o $(USER_LIB_STATIC)
	$(LD) $(LDFLAGS) -T $(USER_LINKER_SCRIPT) -o $@ $^

$(USER_BIN)/%.o: $(USER_BIN)/%.c $(USER_HDRS)
	$(CC) $(CFLAGS) -I$(INCLUDE_DST) -c -o $@ $<



# Emulation

qemu: $(KERNEL_BIN) $(FSIMG)
	$(QEMU) $(QEMUOPTS) -kernel $(KERNEL_BIN) \
		-drive file=$(FSIMG),if=virtio,format=raw

gdb: $(KERNEL_ELF)
	$(GDB) \
	-ex 'target remote localhost:1234' \
	-ex 'b main' \
	-ex 'c' \
	$(KERNEL_ELF)
	

# clean
clean:
	rm -f $(KERNEL_ELF) \
	*.img \
	$(KERNEL)/*.o \
	$(USER_LIB)/*.o \
	$(USER_BIN)/*.o \
	$(LIB_DST)/* \
	$(BIN_DST)/* \
	$(INCLUDE_DST)/*