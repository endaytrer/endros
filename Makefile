PREFIX := riscv64-elf-
CC := $(PREFIX)gcc
LD := $(PREFIX)ld
AS := $(PREFIX)as
GDB := $(PREFIX)gdb
OBJCOPY := $(PREFIX)objcopy

QEMU := qemu-system-riscv64
QEMUOPTS := -machine virt -smp 4 -m 4G
ifdef DEBUG
QEMUOPTS += -s -S
endif

CFLAGS := -ffreestanding -nostdlib -fno-omit-frame-pointer -g -O0

KERNEL := kernel
KERNEL_C_SRCS := $(wildcard $(KERNEL)/*.c)
KERNEL_C_HDRS := $(wildcard $(KERNEL)/*.h)
KERNEL_ASM_SRCS := $(wildcard $(KERNEL)/*.S)

KERNEL_OBJS := $(patsubst $(KERNEL)/%.c, $(KERNEL)/%.o, $(KERNEL_C_SRCS))
KERNEL_OBJS += $(patsubst $(KERNEL)/%.S, $(KERNEL)/%.o, $(KERNEL_ASM_SRCS))

KERNEL_ELF := coreos
KERNEL_BIN := coreos.img
KERNEL_LINKER_SCRIPT := kernel.ld

.PHONY: all clean qemu

all: $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) $^ -O binary $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -T $(KERNEL_LINKER_SCRIPT) -o $@ $(KERNEL_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.S
	$(AS) $(ASFLAGS) -c -o $@ $^

qemu: $(KERNEL_BIN)
	$(QEMU) $(QEMUOPTS) -kernel $(KERNEL_BIN)

gdb: $(KERNEL_ELF)
	$(GDB) \
	-ex 'target remote localhost:1234' \
	-ex 'b main' \
	-ex 'c' \
	$(KERNEL_ELF)
	
clean:
	rm -f $(KERNEL_ELF) $(KERNEL_BIN) $(KERNEL_OBJS)