
.align 3
.section data
.globl app_manager
app_manager:
    .quad 5
    .quad 0
    .quad app_0_start
    .quad app_1_start
    .quad app_2_start
    .quad app_3_start
    .quad app_4_start
    .space 20

    .section .data
    .globl app_0_start
    .globl app_0_end
app_0_start:
    .incbin "1.img"
app_0_end:

    .section .data
    .globl app_1_start
    .globl app_1_end
app_1_start:
    .incbin "2.img"
app_1_end:

    .section .data
    .globl app_2_start
    .globl app_2_end
app_2_start:
    .incbin "3.img"
app_2_end:

    .section .data
    .globl app_3_start
    .globl app_3_end
app_3_start:
    .incbin "4.img"
app_3_end:

    .section .data
    .globl app_4_start
    .globl app_4_end
app_4_start:
    .incbin "5.img"
app_4_end:
