ifndef PREFIX
PREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake PREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif
CC := $(PREFIX)gcc
LD := $(PREFIX)ld
AS := $(PREFIX)as
AR := $(PREFIX)ar
GDB := gf2
OBJCOPY := $(PREFIX)objcopy

# Utils. Compile both to kernel and user lib.
UTIL := utils
UTIL_C_SRCS := $(wildcard $(UTIL)/*.c)
UTIL_C_HDRS := $(wildcard $(UTIL)/*.h)

CFLAGS := -Wall -ffreestanding -nostdlib -fno-omit-frame-pointer -g -I$(UTIL)

QEMU := qemu-system-riscv64
QEMUOPTS := -machine virt
ifdef DEBUG
CFLAGS += -O0
QEMUOPTS += -s -S
else
CLFAGS += -0
endif





# Kernel
KERNEL := kernel
KERNEL_C_SRCS := $(wildcard $(KERNEL)/*.c)
KERNEL_ASM_SRCS := $(wildcard $(KERNEL)/*.S)

CONFIG_HDR := $(KERNEL)/machine.h

KERNEL_DRIVER := $(KERNEL)/drivers
KERNEL_DRIVER_C_SRCS := $(wildcard $(KERNEL_DRIVER)/*.c)
KERNEL_DRIVER_C_HDRS := $(wildcard $(KERNEL_DRIVER)/*.h)

KERNEL_C_HDRS := $(wildcard $(KERNEL)/*.h)
KERNEL_C_HDRS += $(CONFIG_HDR)
KERNEL_C_HDRS += $(UTIL_C_HDRS)
KERNEL_C_HDRS += $(KERNEL_DRIVER_C_HDRS)

KERNEL_OBJS := $(patsubst $(KERNEL)/%.c, $(KERNEL)/%.o, $(KERNEL_C_SRCS))
KERNEL_OBJS += $(patsubst $(KERNEL)/%.S, $(KERNEL)/%.o, $(KERNEL_ASM_SRCS))
KERNEL_OBJS += $(patsubst $(KERNEL_DRIVER)/%.c, $(KERNEL)/%.o, $(KERNEL_DRIVER_C_SRCS))
KERNEL_OBJS += $(patsubst $(UTIL)/%.c, $(KERNEL)/%.o, $(UTIL_C_SRCS))

KERNEL_ELF := endros
KERNEL_BIN := endros.img
KERNEL_LINKER_SCRIPT := $(KERNEL)/kernel.ld

# User Library and Binary

USER := user
USER_LIB := $(USER)/libcoreos
USER_BIN := $(USER)/program
USER_LINKER_SCRIPT := $(USER)/user.ld

USER_LIB_C_SRCS := $(wildcard $(USER_LIB)/*.c)
USER_LIB_C_HDRS := $(wildcard $(USER_LIB)/*.h)
USER_LIB_C_HDRS += $(UTIL_C_HDRS)
USER_LIB_ASM_SRCS := $(wildcard $(USER_LIB)/*.S)

USER_LIB_OBJS := $(patsubst $(USER_LIB)/%.c, $(USER_LIB)/%.o, $(USER_LIB_C_SRCS))
USER_LIB_OBJS += $(patsubst $(USER_LIB)/%.S, $(USER_LIB)/%.o, $(USER_LIB_ASM_SRCS))
USER_LIB_OBJS += $(patsubst $(UTIL)/%.c, $(USER_LIB)/%.o, $(UTIL_C_SRCS))

USER_BIN_SRCS := $(wildcard $(USER_BIN)/*.c)
USER_BIN_OBJS := $(patsubst $(USER_BIN)/%.c, $(USER_BIN)/%.o, $(USER_BIN_SRCS))


# File System
FSIMG := fs.img
ROOTFS := rootfs
BIN_DST := $(ROOTFS)/bin
LIB_DST := $(ROOTFS)/lib
INCLUDE_DST := $(ROOTFS)/include

USER_ELFS := $(patsubst $(USER_BIN)/%.o, $(BIN_DST)/%, $(USER_BIN_OBJS))
USER_LIB_STATIC := $(LIB_DST)/libcoreos.a
USER_HDRS := $(patsubst $(USER_LIB)/%.h, $(INCLUDE_DST)/%.h, $(USER_LIB_C_HDRS))

SCRIPT := script
MKFS := $(SCRIPT)/mkfs.py
GENHDRS := $(SCRIPT)/genhdrs.py

DEFCONF := defconf
CONFIG := .config

.PHONY: all qemu libcoreos kernel user defconfig cleanall clean cleanconfig

all: kernel user

# Configuration


defconfig:
	cp $(DEFCONF) $(CONFIG)
	$(GENHDRS) -o $(CONFIG_HDR) -c $(CONFIG)

# Compiling kernel
kernel: $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) $^ -O binary $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -T $(KERNEL_LINKER_SCRIPT) -o $@ $(KERNEL_OBJS)

$(KERNEL)/%.o: $(KERNEL)/%.c $(KERNEL_C_HDRS)
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(KERNEL)/%.o: $(KERNEL)/%.S $(KERNEL_C_HDRS)
	$(AS) $(ASFLAGS) -c -o $@ $<

$(KERNEL)/%.o: $(UTIL)/%.c $(UTIL_C_HDRS)
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(KERNEL)/%.o: $(KERNEL_DRIVER)/%.c $(KERNEL_C_HDRS)
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(CONFIG_HDR): $(CONFIG) $(GENHDRS)
	$(GENHDRS) -o $@ -c $<

$(CONFIG):
	@echo "\nYou should run 'make defconfig' to build default configuration, and then edit it.\n"
	@exit 1
# User library

libcoreos: $(USER_LIB_STATIC)

$(USER_LIB_STATIC): $(USER_LIB_OBJS) | $(LIB_DST)
	$(AR) rcs $@ $^

$(USER_LIB)/%.o: $(USER_LIB)/%.c
	$(CC) -fPIC $(CFLAGS) -c -o $@ $^

$(USER_LIB)/%.o: $(USER_LIB)/%.S
	$(AS) -fPIC $(ASFLAGS) -c -o $@ $^

$(USER_LIB)/%.o: $(UTIL)/%.c $(UTIL_C_HDRS)
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(INCLUDE_DST)/%.h: $(USER_LIB)/%.h | $(INCLUDE_DST)
	cp $^ $@

$(LIB_DST):
	mkdir -p $@

$(INCLUDE_DST):
	mkdir -p $@

$(BIN_DST):
	mkdir -p $@

# User programs
user: $(FSIMG)

$(FSIMG): $(USER_HDRS) $(USER_ELFS) $(USER_LIB_STATIC) $(CONFIG) $(MKFS)
	$(MKFS) -o $@ $(ROOTFS) -s $(shell $(GENHDRS) -g FSSIZE -c $(CONFIG)) -i $(shell $(GENHDRS) -g INODES -c $(CONFIG))

$(BIN_DST)/%: $(USER_BIN)/%.o $(USER_LIB_STATIC) | $(BIN_DST)
	$(LD) $(LDFLAGS)  -o $@ $^

$(USER_BIN)/%.o: $(USER_BIN)/%.c $(USER_HDRS)
	$(CC) $(CFLAGS) -I$(INCLUDE_DST) -c -o $@ $<



# Emulation
qemu: $(CONFIG) $(FSIMG) $(KERNEL_BIN)
	$(QEMU) $(QEMUOPTS) -kernel $(KERNEL_BIN) \
		-smp $(shell $(GENHDRS) -g NCPU -c $(CONFIG)) \
		-m $(shell $(GENHDRS) -g MEM_SIZE -c $(CONFIG)) \
		-global virtio-mmio.force-legacy=false \
		-drive file=$(FSIMG),if=none,format=raw,id=x0 \
		-device virtio-blk-device,drive=x0 \
		-device virtio-gpu-device

gdb: $(KERNEL_ELF)
	$(GDB) \
	-ex 'target remote localhost:1234' \
	-ex 'b init' \
	-ex 'c' \
	$(KERNEL_ELF)
	

# clean
cleanall: clean cleanconfig

clean:
	rm -f $(KERNEL_ELF) \
	*.img \
	$(KERNEL)/*.o \
	$(USER_LIB)/*.o \
	$(USER_BIN)/*.o \
	$(LIB_DST)/* \
	$(BIN_DST)/* \
	$(INCLUDE_DST)/*

cleanconfig:
	rm -f $(CONFIG) $(CONFIG_HDR)
