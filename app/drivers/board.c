#include <board.h>
#include <mmu.h>
#include <gic.h>
#include <drv_uart.h>
#include <drv_clk.h>

struct mem_desc platform_mem_desc[] = {
    {0x00000000, 0xFFFFFFFF, 0x00000000, DEVICE_MEM},
    {0x40000000, 0x47FFFFFF, 0x40000000, NORMAL_MEM},
};

const rt_uint32_t platform_mem_desc_size = sizeof(platform_mem_desc)/sizeof(platform_mem_desc[0]);

#define SYS_TIMER_BASE      0x02050000
#define SYS_TIMER_SRC_CLK   24000000

static void systick_timer_isr(int vector, void *param)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    writel(readl(SYS_TIMER_BASE + 0x04), SYS_TIMER_BASE + 0x04); // clear interrupt

    /* leave interrupt */
    rt_interrupt_leave();
}

static void systick_timer_init(void)
{
    rt_uint32_t base = SYS_TIMER_BASE;
    rt_uint32_t reg_val;

    /* select timer0 clock src */
    reg_val = readl(base + 0x10);
    reg_val &= ~(0x3 << 3);
    reg_val |= (0x1 << 3);      /* select 24MHz clock */
    reg_val &= ~(0x1 << 7);     /* set timer mode periodic */
    reg_val &= ~(0x7 << 4);     /* set timer pres 1 */
    reg_val |= (0x1 << 1);      /* enable timer reload */
    writel(reg_val, base + 0x10);

    /* select timer1 clock src */
    reg_val = readl(base + 0x20);
    reg_val &= ~(0x3 << 3);
    reg_val |= (0x1 << 3);      /* select 24MHz clock */
    reg_val &= ~(0x1 << 7);     /* set timer mode periodic */
    reg_val &= ~(0x7 << 4);     /* set timer pres 1 */
    reg_val |= (0x1 << 1);      /* enable timer reload */
    writel(reg_val, base + 0x20);

    /* set timer0 reload value */
    reg_val = (SYS_TIMER_SRC_CLK) / RT_TICK_PER_SECOND;
    writel(reg_val, base + 0x14);

    /* set timer1 reload value */
    reg_val = 0xffffffff;
    writel(reg_val, base + 0x24);

    /* enable irq and start timer */
    reg_val = readl(base + 0x00);
    reg_val |= (0x1 << 0);      /* enable timer interrupt */
    writel(reg_val, base + 0x00);

    rt_hw_interrupt_install(91, systick_timer_isr, RT_NULL, "systimer0");
    rt_hw_interrupt_umask(91);

    reg_val = readl(base + 0x10);
    reg_val |= (0x1 << 0);      /* enable timer0 */
    writel(reg_val, base + 0x10);

    reg_val = readl(base + 0x20);
    reg_val |= (0x1 << 0);      /* enable timer1 */
    writel(reg_val, base + 0x20);
}

void rt_hw_us_delay(rt_uint32_t us)
{
    rt_uint32_t ticks = us * 24;
    rt_uint32_t start = readl(SYS_TIMER_BASE + 0x28); /* read timer1 current tick */
    rt_uint32_t now;

    while (1)
    {
        now = readl(SYS_TIMER_BASE + 0x28);
        if ((start - now) >= ticks)
        {
            break;
        }
    }
}

/**
 *  enable ACTLR SMP, it must be enable for MMU & CACHE
 */
static void set_actlr(void)
{
    rt_uint32_t actlr = 0;

    asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r" (actlr));
    actlr |= (1 << 6);
    asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r" (actlr) : "memory");
}

void idle_wfi(void)
{
    asm volatile ("wfi");
}

/**
 * This function will initialize beaglebone board
 */
void rt_hw_board_init(void)
{
    /* enable ACTLR SMP for enable for MMU & CACHE */
    set_actlr();

    /* init ccu with default configure */
    drv_clk_init();

    /* initialize hardware interrupt */
    rt_hw_interrupt_init();

    rt_hw_uart_init();

    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

    /* initialize system heap */
    rt_system_heap_init(HEAP_BEGIN, HEAP_END);

    systick_timer_init();

#ifdef RT_USING_SMP
    /* install IPI handle */
    extern void rt_hw_ipi_handler_install(int ipi_vector, rt_isr_handler_t ipi_isr_handler);
    rt_hw_ipi_handler_install(RT_SCHEDULE_IPI, rt_scheduler_ipi_handler);
    rt_hw_interrupt_umask(RT_SCHEDULE_IPI);
#endif

    rt_thread_idle_sethook(idle_wfi);

#if defined(RT_USING_COMPONENTS_INIT)
    rt_components_board_init();
#endif /* RT_USING_COMPONENTS_INIT */
}

#ifdef RT_USING_SMP
void rt_hw_secondary_cpu_up(void)
{
    /*
    The Soft Entry Address Register of CPU0 is 0x070005C4.
    The Soft Entry Address Register of CPU1 is 0x070005C8.
    */
    rt_uint32_t cpuboot_membase = 0x070005c4;
    rt_uint32_t cpuxcfg_membase = 0x09010000;
    rt_uint32_t reg;
    rt_uint8_t i = 0;

    for (i = 1; i < RT_CPUS_NR; i++)
    {
        /* Set CPU boot address */
        extern void secondary_cpu_start(void);
        writel((uint32_t)(secondary_cpu_start), cpuboot_membase + 4 * i);

        /* Deassert the CPU core in reset */
        reg = readl(cpuxcfg_membase);
        writel(reg | (1 << i), cpuxcfg_membase);
    }

    __asm__ volatile ("dsb":::"memory");

    for (i = 1; i < RT_CPUS_NR; i++)
    {
        rt_hw_ipi_send(RT_SCHEDULE_IPI, (RT_CPU_MASK ^ (1 << i)));
    }
}

void secondary_cpu_c_start(void)
{
    rt_kprintf("CPU %d start\n", rt_hw_cpu_id());

    extern void rt_hw_vector_init(void);
    rt_hw_vector_init();

    rt_hw_spin_lock(&_cpus_lock);

    arm_gic_cpu_init(0, platform_get_gic_cpu_base());

    systick_timer_init();

    rt_system_scheduler_start();

    while(1);
}

void rt_hw_secondary_cpu_idle_exec(void)
{
    asm volatile ("wfe":::"memory", "cc");
}

#endif
