.section .text.entry
.globl _start
.extern main
.extern __alltraps
_start:
    la sp, boot_stack_top

    # set trap vector
    la t0, __alltraps
    csrw stvec, t0
    call main
    j .

.global kernel_stack_top
.global user_stack_top
.section .bss.stack
    .space 0x8000
boot_stack_top:
    .space 0x8000
kernel_stack_top:
    .space 0x10000
user_stack_top:
