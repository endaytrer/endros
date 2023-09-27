.section .text.entry
.globl _start
.extern init
_start:
    la sp, boot_stack_top
    call init
    j .

.section .bss.stack
    .space 0x1000
boot_stack_top:
