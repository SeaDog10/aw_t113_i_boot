.section .vectors, "ax"
.code 32

.long 0x12345678
.long __img_size
.long __img_start
.long __entry_addr
.long 0x00000000
.long 0x11111111
.long 0x22222222
.long 0x33333333
.long 0x44444444
.long 0x55555555
.long 0x66666666
.long 0x77777777
.long 0x88888888
.long 0x99999999
.long 0xaaaaaaaa
.long 0xbbbbbbbb

.globl system_vectors
system_vectors:
    ldr pc, _vector_reset
    ldr pc, _vector_undef
    ldr pc, _vector_swi
    ldr pc, _vector_pabt
    ldr pc, _vector_dabt
    ldr pc, _vector_resv
    ldr pc, _vector_irq
    ldr pc, _vector_fiq

.globl _reset
.globl vector_undef
.globl vector_swi
.globl vector_pabt
.globl vector_dabt
.globl vector_resv
.globl vector_irq
.globl vector_fiq

_vector_reset:
    .word _reset
_vector_undef:
    .word vector_undef
_vector_swi:
    .word vector_swi
_vector_pabt:
    .word vector_pabt
_vector_dabt:
    .word vector_dabt
_vector_resv:
    .word vector_resv
_vector_irq:
    .word vector_irq
_vector_fiq:
    .word vector_fiq

.balignl 	16,0xdeadbeef