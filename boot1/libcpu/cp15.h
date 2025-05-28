#ifndef __CP15_H__
#define __CP15_H__

#define __get_cp(cp, op1, Rt, CRn, CRm, op2) __asm__ volatile("MRC p" # cp ", " # op1 ", %0, c" # CRn ", c" # CRm ", " # op2 : "=r" (Rt) : : "memory" )
#define __set_cp(cp, op1, Rt, CRn, CRm, op2) __asm__ volatile("MCR p" # cp ", " # op1 ", %0, c" # CRn ", c" # CRm ", " # op2 : : "r" (Rt) : "memory" )
#define __get_cp64(cp, op1, Rt, CRm) __asm__ volatile("MRRC p" # cp ", " # op1 ", %Q0, %R0, c" # CRm  : "=r" (Rt) : : "memory" )
#define __set_cp64(cp, op1, Rt, CRm) __asm__ volatile("MCRR p" # cp ", " # op1 ", %Q0, %R0, c" # CRm  : : "r" (Rt) : "memory" )

void dcache_enable(void);
void icache_enable(void);
void dcache_disable(void);
void icache_disable(void);

void dcache_clean_flush(void);
void icache_flush(void);

void mmu_disable(void);
void mmu_enable(void);
void tlb_set(volatile unsigned long*);

void vector_set_base(unsigned int addr);

void interrupt_disable(void);
void interrupt_enable(void);

#endif