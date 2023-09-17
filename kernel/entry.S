.section .text.entry
.globl _start
.extern main
_start:
    la sp, boot_stack_top
    call main
    j .

.section .bss.stack
    .space 0x10000
boot_stack_top:
