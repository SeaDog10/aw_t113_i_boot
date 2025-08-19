#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>

/* BSP version information */
#define T113_GIC_CPU_BASE       (0x03022000)
#define T113_GIC_DIST_BASE      (0x03021000)

#define UNCACHE_MEM_ADDR        (0x40000000)
#define UNCACHE_MEM_SIZE        (0x00100000)

/* t113 on-board gic irq sources */
#define ARM_GIC_NR_IRQS         223
#define MAX_HANDLERS            223

/* only one GIC available */
#define ARM_GIC_MAX_NR          1
#define GIC_IRQ_START           0
#define GIC_ACK_INTID_MASK      0x000003ff

#define DDR_START               (0x40000000)
#define DDR_END                 (0x50000000)
#define T113_DDR_SIZE           ((DDR_END - DDR_START))

#define readb(reg)                          (*((volatile unsigned char *)(reg)))
#define readw(reg)                          (*((volatile unsigned short *)(reg)))
#define readl(reg)                          (*((volatile unsigned int *)(reg)))
#define readq(reg)                          (*((volatile unsigned long long *)(reg)))

#define writeb(data, reg)                   ((*((volatile unsigned char *)(reg))) = (unsigned char)(data))
#define writew(data, reg)                   ((*((volatile unsigned short *)(reg))) = (unsigned short)(data))
#define writel(data, reg)                   ((*((volatile unsigned int *)(reg))) = (unsigned int)(data))
#define writeq(data, reg)                   ((*((volatile unsigned long long *)(reg))) = (unsigned long long)(data))

#define __REG32(x)                          (*((volatile unsigned int *)(x)))
#define __REG16(x)                          (*((volatile unsigned short *)(x)))

/* the basic constants and interfaces needed by gic */
rt_inline rt_uint32_t platform_get_gic_dist_base(void)
{
    return T113_GIC_DIST_BASE;
}

rt_inline rt_uint32_t platform_get_gic_cpu_base(void)
{
    return T113_GIC_CPU_BASE;
}

extern int _heap_start;
#define HEAP_BEGIN                ((void *)&_heap_start)
extern int _heap_end;
#define HEAP_END                  ((void *)&_heap_end)

void rt_hw_board_init(void);
void rt_hw_us_delay(rt_uint32_t us);

void *uncache_malloc(rt_size_t size);
void  uncache_free(void *rmem);
void *uncache_malloc_align(rt_size_t size, rt_size_t align);
void uncache_free_align(void *ptr);

#endif
