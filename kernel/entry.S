.section .text.entry
.globl _start
.extern main
_start:
    la sp, boot_stack_top
    call main
    j .

.global kernel_stack_top
.global user_stack_top
.section .bss.stack
    .space 0x4000
boot_stack_top:
