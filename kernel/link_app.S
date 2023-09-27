.align 3
.section .data
.global _num_app
.global name_app_map
_num_app:

    .quad 5
    .quad init_start
    # table for name - address
name_app_map:
    .quad name_0
    .quad app_0_start
    .quad name_1
    .quad app_1_start
    .quad name_2
    .quad app_2_start
    .quad name_3
    .quad app_3_start
    .quad name_4
    .quad app_4_start
    
.section .rodata

init_start:
    .incbin "rootfs/bin/init"
name_0:
    .asciz "/bin/1"
app_0_start:
    .incbin "rootfs/bin/1"

name_1:
    .asciz "/bin/2"
app_1_start:
    .incbin "rootfs/bin/2"

name_2:
    .asciz "/bin/3"
app_2_start:
    .incbin "rootfs/bin/3"

name_3:
    .asciz "/bin/4"
app_3_start:
    .incbin "rootfs/bin/4"

name_4:
    .asciz "/bin/5"
app_4_start:
    .incbin "rootfs/bin/5"
