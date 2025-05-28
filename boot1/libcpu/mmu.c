#include "mmu.h"
#include "cp15.h"

/* level1 page table, each entry for 1MB memory. */
volatile static unsigned long MMUTable[4*1024] __attribute__((aligned(16*1024)));
void mmu_setmtt(unsigned int vaddrStart,
                      unsigned int vaddrEnd,
                      unsigned int paddrStart,
                      unsigned int attr)
{
    volatile unsigned int *pTT;
    volatile int i, nSec;
    pTT  = (unsigned int *)MMUTable + (vaddrStart >> 20);
    nSec = (vaddrEnd >> 20) - (vaddrStart >> 20);
    for(i = 0; i <= nSec; i++)
    {
        *pTT = attr | (((paddrStart >> 20) + i) << 20);
        pTT++;
    }
}

unsigned long set_domain_register(unsigned long domain_val)
{
    unsigned long old_domain;

    asm volatile ("mrc p15, 0, %0, c3, c0\n" : "=r" (old_domain));
    asm volatile ("mcr p15, 0, %0, c3, c0\n" : :"r" (domain_val) : "memory");

    return old_domain;
}

void init_mmu_table(struct mem_desc *mdesc, unsigned int size)
{
    /* set page table */
    for(; size > 0; size--)
    {
        mmu_setmtt(mdesc->vaddr_start, mdesc->vaddr_end, mdesc->paddr_start, mdesc->attr);
        mdesc++;
    }
}

void mmu_init(void)
{
    dcache_clean_flush();
    icache_flush();
    dcache_disable();
    icache_disable();
    mmu_disable();

    /*rt_hw_cpu_dump_page_table(MMUTable);*/
    set_domain_register(0x55555555);

    tlb_set(MMUTable);

    mmu_enable();

    icache_enable();
    dcache_enable();
}
