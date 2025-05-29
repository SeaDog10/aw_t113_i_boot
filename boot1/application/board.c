#include "mmu.h"

struct mem_desc platform_mem_desc[] =
{
    {0x00000000, 0x00007FFF, 0x00000000, DEVICE_MEM}, /* SRAM A1    */
    {0x02000000, 0x04FFFFFF, 0x02000000, DEVICE_MEM}, /* PERIPHERAL */
    {0x40000000, 0x4FFFFFFF, 0x40000000, NORMAL_MEM}, /* DRAM       */
};

const unsigned int platform_mem_desc_size = sizeof(platform_mem_desc)/sizeof(platform_mem_desc[0]);
