#include "mmu.h"

struct mem_desc platform_mem_desc[] =
{
    {0x00000000, 0x00007FFF, 0x00000000, DEVICE_MEM}, /* SRAM A1    */
    {0x02000000, 0x04FFFFFF, 0x02000000, DEVICE_MEM}, /* PERIPHERAL */
    {0x40000000, 0x4FFFFFFF, 0x40000000, NORMAL_MEM}, /* DRAM       */
};

const unsigned int platform_mem_desc_size = sizeof(platform_mem_desc)/sizeof(platform_mem_desc[0]);

/*
 * 64bit arch timer.CNTPCT
 * Freq = 24000000Hz
 */
unsigned long long get_gtimer_count(void)
{
    unsigned int lo, hi;

    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi) : : "memory");

    return (((unsigned long long)hi << 32) | ((unsigned long long)lo));
}

/*
 * get current time.(millisecond)
 */
unsigned long long get_count_ms(void)
{
    return get_gtimer_count() / (unsigned long long)(24 * 1000);
}

/*
 * get current time.(microsecond)
 */
unsigned long long get_count_us(void)
{
    return get_gtimer_count() / (unsigned long long)24;
}

void us_delay(unsigned long long us)
{
    unsigned long long now;

    now = get_count_us();

    while (get_count_us() - now < us);
}

void ms_delay(unsigned long long ms)
{
    unsigned long long now;

    now = get_count_ms();

    while (get_count_ms() - now < ms);
}
