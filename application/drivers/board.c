#include <board.h>
#include <mmu.h>
#include <gic.h>
#include <drv_uart.h>
#include <drv_clk.h>

static struct rt_memheap uncache_memheap = {0};
static struct rt_mutex _mem_lock;

rt_inline void _mem_heap_lock_init(void)
{
    rt_mutex_init(&_mem_lock, "heap", RT_IPC_FLAG_PRIO);
}

rt_inline rt_base_t _mem_heap_lock(void)
{
    if (rt_thread_self())
        return rt_mutex_take(&_mem_lock, RT_WAITING_FOREVER);
    else
        return RT_EOK;
}

rt_inline void _mem_heap_unlock(rt_base_t level)
{
    RT_ASSERT(level == RT_EOK);
    if (rt_thread_self())
        rt_mutex_release(&_mem_lock);
}

void *uncache_malloc(rt_size_t size)
{
    rt_base_t level;
    void *ptr;

    level = _mem_heap_lock();

    ptr = rt_memheap_alloc(&uncache_memheap, size);

    _mem_heap_unlock(level);

    return ptr;
}

void uncache_free(void *rmem)
{
    rt_base_t level;

    if (rmem == RT_NULL) return;
    level = _mem_heap_lock();
    rt_memheap_free(rmem);
    _mem_heap_unlock(level);
}

void *uncache_malloc_align(rt_size_t size, rt_size_t align)
{
    void *ptr;
    void *align_ptr;
    int uintptr_size;
    rt_size_t align_size;

    /* sizeof pointer */
    uintptr_size = sizeof(void*);
    uintptr_size -= 1;

    /* align the alignment size to uintptr size byte */
    align = ((align + uintptr_size) & ~uintptr_size);

    /* get total aligned size */
    align_size = ((size + uintptr_size) & ~uintptr_size) + align;
    /* allocate memory block from heap */
    ptr = uncache_malloc(align_size);
    if (ptr != RT_NULL)
    {
        /* the allocated memory block is aligned */
        if (((rt_ubase_t)ptr & (align - 1)) == 0)
        {
            align_ptr = (void *)((rt_ubase_t)ptr + align);
        }
        else
        {
            align_ptr = (void *)(((rt_ubase_t)ptr + (align - 1)) & ~(align - 1));
        }

        /* set the pointer before alignment pointer to the real pointer */
        *((rt_ubase_t *)((rt_ubase_t)align_ptr - sizeof(void *))) = (rt_ubase_t)ptr;

        ptr = align_ptr;
    }

    return ptr;
}

void uncache_free_align(void *ptr)
{
    void *real_ptr;

    /* NULL check */
    if (ptr == RT_NULL) return;
    real_ptr = (void *) * (rt_ubase_t *)((rt_ubase_t)ptr - sizeof(void *));
    uncache_free(real_ptr);
}

static void uncache_mem_info(void)
{
    rt_size_t total, used, max_used;

    rt_memheap_info(&uncache_memheap, &total, &used, &max_used);

    rt_kprintf("heap      name : %s\n", uncache_memheap.parent.name);
    rt_kprintf("total     size : 0x%08x\n", total);
    rt_kprintf("used      size : 0x%08x\n", used);
    rt_kprintf("available size : 0x%08x\n", total - used);
    rt_kprintf("max used  size : 0x%08x\n", max_used);
}
MSH_CMD_EXPORT(uncache_mem_info, uncache_mem_info);

struct mem_desc platform_mem_desc[] = {
    {0x00000000, 0xFFFFFFFF, 0x00000000, DEVICE_MEM},
    {0x40100000, 0x47FFFFFF, 0x40100000, NORMAL_MEM},
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

#ifdef RT_AMP_MASTER
void rt_hw_slave_cpu_up(void)
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
        writel((uint32_t)(0x40200000), cpuboot_membase + 4 * i);

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
#endif /* RT_AMP_MASTER */

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

    /* initialize uncache mem */
    rt_mutex_init(&_mem_lock, "uncachemem", RT_IPC_FLAG_PRIO);
    rt_memheap_init(&uncache_memheap, "uncachemem", (void *)UNCACHE_MEM_ADDR, (rt_size_t)UNCACHE_MEM_SIZE);

    systick_timer_init();

#ifdef RT_USING_SMP
    /* install IPI handle */
    extern void rt_hw_ipi_handler_install(int ipi_vector, rt_isr_handler_t ipi_isr_handler);
    rt_hw_ipi_handler_install(RT_SCHEDULE_IPI, rt_scheduler_ipi_handler);
    rt_hw_interrupt_umask(RT_SCHEDULE_IPI);
#endif

    rt_thread_idle_sethook(idle_wfi);

#ifdef RT_AMP_MASTER
    rt_hw_slave_cpu_up();
#endif /* RT_AMP_MASTER */

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
