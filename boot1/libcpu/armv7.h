#ifndef __ARMV7_H__
#define __ARMV7_H__

/* the exception stack without VFP registers */
struct exp_stack
{
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int fp;
    unsigned int ip;
    unsigned int sp;
    unsigned int lr;
    unsigned int pc;
    unsigned int cpsr;
};

struct stack
{
    unsigned int cpsr;
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int fp;
    unsigned int ip;
    unsigned int lr;
    unsigned int pc;
};

#define USERMODE    0x10
#define FIQMODE     0x11
#define IRQMODE     0x12
#define SVCMODE     0x13
#define MONITORMODE 0x16
#define ABORTMODE   0x17
#define HYPMODE     0x1b
#define UNDEFMODE   0x1b
#define MODEMASK    0x1f
#define NOINT       0xc0

#define T_Bit       (1<<5)
#define F_Bit       (1<<6)
#define I_Bit       (1<<7)
#define A_Bit       (1<<8)
#define E_Bit       (1<<9)
#define J_Bit       (1<<24)

#define PABT_EXCEPTION 0x1
#define DABT_EXCEPTION 0x2
#define UND_EXCEPTION  0x3
#define SWI_EXCEPTION  0x4
#define RESV_EXCEPTION 0xF

#endif
