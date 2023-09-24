.align 3
.section .data
.global _num_app

_num_app:

    .quad 5
    .quad app_0_start
    .quad app_1_start
    .quad app_2_start
    .quad app_3_start
    .quad app_4_start
    .quad app_4_end
    
.section .data
.global app_0_start
.global app_0_end

app_0_start:
    .incbin "rootfs/bin/1.bin"
app_0_end:

.section .data
.global app_1_start
.global app_1_end
app_1_start:
    .incbin "rootfs/bin/2.bin"
app_1_end:

.section .data
.global app_2_start
.global app_2_end
app_2_start:
    .incbin "rootfs/bin/3.bin"
app_2_end:


.section .data
.global app_3_start
.global app_3_end
app_3_start:
    .incbin "rootfs/bin/4.bin"
app_3_end:

.section .data
.global app_4_start
.global app_4_end
app_4_start:
    .incbin "rootfs/bin/5.bin"
app_4_end:
