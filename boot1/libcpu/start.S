.equ Mode_USR,        0x10
.equ Mode_FIQ,        0x11
.equ Mode_IRQ,        0x12
.equ Mode_SVC,        0x13
.equ Mode_ABT,        0x17
.equ Mode_UND,        0x1B
.equ Mode_SYS,        0x1F

.equ I_Bit,           0x80            @ when I bit is set, IRQ is disabled
.equ F_Bit,           0x40            @ when F bit is set, FIQ is disabled

.equ UND_Stack_Size,     0x00001000
.equ SVC_Stack_Size,     0x00001000
.equ ABT_Stack_Size,     0x00001000
.equ RT_FIQ_STACK_PGSZ,  0x00001000
.equ RT_IRQ_STACK_PGSZ,  0x00001000
.equ USR_Stack_Size,     0x00001000

#define ISR_Stack_Size  (UND_Stack_Size + SVC_Stack_Size + ABT_Stack_Size + \
                 RT_FIQ_STACK_PGSZ + RT_IRQ_STACK_PGSZ)

.section .bss.share.isr
/* stack */
.globl stack_start
.globl stack_top

.align 3
stack_start:
.rept ISR_Stack_Size
.byte 0
.endr
stack_top:

.text
.globl _reset
_reset:
    /* set the cpu to SVC32 mode and disable interrupt */
    cps #Mode_SVC
    /* disable the data alignment check */
    mrc p15, 0, r1, c1, c0, 0
    bic r1, #(1<<0)             /* Disable MMU */
    bic r1, #(1<<1)             /* Disable Alignment fault checking */
    bic r1, #(1<<2)             /* Disable data cache */
    bic r1, #(1<<11)            /* Disable program flow prediction */
    bic r1, #(1<<12)            /* Disable instruction cache */
    bic r1, #(3<<19)            /* bit[20:19] must be zero */
    mcr p15, 0, r1, c1, c0, 0

    bl      stack_setup

    /* clear .bss */
    mov     r0,#0                   /* get a zero                       */
    ldr     r1,=__bss_start         /* bss start                        */
    ldr     r2,=__bss_end           /* bss end                          */

bss_loop:
    cmp     r1,r2                   /* check if data to clear           */
    strlo   r0,[r1],#4              /* clear 4 bytes                    */
    blo     bss_loop                /* loop until done                  */

    /* enable branch prediction */
    mrc p15, 0, r0, c1, c0, 0
    orr     r0, r0, #(1<<11)
    mcr p15, 0, r0, c1, c0, 0

    /* initialize the mmu table and enable mmu */
    ldr r0, =platform_mem_desc
    ldr r1, =platform_mem_desc_size
    ldr r1, [r1]
    bl init_mmu_table
    bl mmu_init

    /* start RT-Thread Kernel */
    ldr     pc, _startup
_startup:
    .word main

stack_setup:
    ldr     r0, =stack_top

    @  Set the startup stack for svc
    mov     sp, r0
    sub     r0, r0, #SVC_Stack_Size

    @  Enter Undefined Instruction Mode and set its Stack Pointer
    msr     cpsr_c, #Mode_UND|I_Bit|F_Bit
    mov     sp, r0
    sub     r0, r0, #UND_Stack_Size

    @  Enter Abort Mode and set its Stack Pointer
    msr     cpsr_c, #Mode_ABT|I_Bit|F_Bit
    mov     sp, r0
    sub     r0, r0, #ABT_Stack_Size

    @  Enter FIQ Mode and set its Stack Pointer
    msr     cpsr_c, #Mode_FIQ|I_Bit|F_Bit
    mov     sp, r0
    sub     r0, r0, #RT_FIQ_STACK_PGSZ

    @  Enter IRQ Mode and set its Stack Pointer
    msr     cpsr_c, #Mode_IRQ|I_Bit|F_Bit
    mov     sp, r0
    sub     r0, r0, #RT_IRQ_STACK_PGSZ

    /* come back to SVC mode */
    msr     cpsr_c, #Mode_SVC|I_Bit|F_Bit
    bx      lr

.section .text.isr, "ax"

    .align  5
    .globl  vector_fiq
vector_fiq:
    stmfd   sp!,{r0-r7,lr}
    bl      trap_fiq
    ldmfd   sp!,{r0-r7,lr}
    subs    pc, lr, #4

    .align  5
    .globl  vector_irq
vector_irq:
    stmfd   sp!, {r0-r12,lr}
    bl      trap_irq
    ldmfd   sp!, {r0-r12,lr}
    subs    pc,  lr, #4

.macro push_svc_reg
    sub     sp, sp, #17 * 4         @/* Sizeof(struct exp_stack)  */
    stmia   sp, {r0 - r12}          @/* Calling r0-r12                  */
    mov     r0, sp
    mrs     r6, spsr                @/* Save CPSR                       */
    str     lr, [r0, #15*4]         @/* Push PC                         */
    str     r6, [r0, #16*4]         @/* Push CPSR                       */
    mrs     r5, cpsr                @/* Save CPSR                       */

    and     r4, r6, #0x1F
    cmp     r4, #Mode_USR
    moveq   r6, #Mode_SYS

    orr     r6, r6, #0x80           @/* Switch to previous mode, then save SP & PC */
    msr     cpsr_c, r6
    str     sp, [r0, #13*4]         @/* Save calling SP                 */
    str     lr, [r0, #14*4]         @/* Save calling PC                 */

    msr     cpsr_c, r5              @/* Switch back to current mode */
.endm

    .align  5
    .weak   vector_swi
vector_swi:
    push_svc_reg
    bl      trap_swi
    b       .

    .align  5
    .globl  vector_undef
vector_undef:
    push_svc_reg
    cps #Mode_UND
    bl      trap_undef
    b       .

    .align  5
    .globl  vector_pabt
vector_pabt:
    push_svc_reg
    bl      trap_pabt
    b       .

    .align  5
    .globl  vector_dabt
vector_dabt:
    push_svc_reg
    bl      trap_dabt
    b       .

    .align  5
    .globl  vector_resv
vector_resv:
    push_svc_reg
    bl      trap_resv
    b       .
