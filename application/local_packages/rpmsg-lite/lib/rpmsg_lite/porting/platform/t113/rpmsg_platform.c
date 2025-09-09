#include <stdio.h>
#include <string.h>

#include "rpmsg_platform.h"
#include "rpmsg_env.h"

#include "rthw.h"
#include "rtthread.h"
#include "interrupt.h"

#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
#error "This RPMsg-Lite port requires RL_USE_ENVIRONMENT_CONTEXT set to 0"
#endif

static rt_base_t level = 0;
static int32_t isr_counter     = 0;
static int32_t disable_counter = 0;
static void *platform_lock;
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
static LOCK_STATIC_CONTEXT platform_lock_static_ctxt;
#endif

static void rpmsg_isr(int irqn, void *param)
{
    uint32_t vq_id = (rt_base_t)param;
    rt_kprintf("rpmsg_isr irqn %d vq_id %d\n", irqn, vq_id);
    rt_interrupt_enter();
    env_isr(vq_id);
    rt_interrupt_leave();
}

static void platform_global_isr_disable(void)
{
    level = rt_hw_interrupt_disable();
}

static void platform_global_isr_enable(void)
{
    rt_hw_interrupt_enable(level);
}

int32_t platform_init_interrupt(uint32_t vector_id, void *isr_data)
{
    uint32_t link_id;

    rt_kprintf("platform_init_interrupt vector_id %d isr_data 0x%x\n", vector_id, isr_data);
    if (platform_lock != ((void *)0))
    {
        /* Register ISR to environment layer */
        env_register_isr(vector_id, isr_data);

        env_lock_mutex(platform_lock);

        RL_ASSERT(0 <= isr_counter);
        if (isr_counter < 2 * 1)
        {
            rt_hw_interrupt_install(vector_id, rpmsg_isr, (void *)vector_id, "rpmsg-lite");
        }
        isr_counter++;

        env_unlock_mutex(platform_lock);
        return 0;
    }
    else
    {
        return -1;
    }
}

int32_t platform_deinit_interrupt(uint32_t vector_id)
{
    if (platform_lock != ((void *)0))
    {
        env_lock_mutex(platform_lock);

        RL_ASSERT(0 < isr_counter);
        isr_counter--;
        if (isr_counter < 2)
        {
        }

        /* Unregister ISR from environment layer */
        env_unregister_isr(vector_id);

        env_unlock_mutex(platform_lock);

        return 0;
    }
    else
    {
        return -1;
    }
}

void platform_notify(uint32_t vector_id)
{
    rt_int32_t cpu_id = rt_hw_cpu_id();
    uint32_t target_cpu;

    env_lock_mutex(platform_lock);

    if (cpu_id == RL_GET_M_CPU_ID)
    {
        rt_hw_ipi_send(vector_id, (RT_CPU_MASK ^ (1 << 0)));
    }
    else
    {
        rt_hw_ipi_send(vector_id, (RT_CPU_MASK ^ (1 << 1)));
    }

    env_unlock_mutex(platform_lock);
}

/**
 * platform_time_delay
 *
 * @param num_msec Delay time in ms.
 *
 * This is not an accurate delay, it ensures at least num_msec passed when return.
 */
void platform_time_delay(uint32_t num_msec)
{
    rt_thread_mdelay(num_msec);
}

/**
 * platform_in_isr
 *
 * Return whether CPU is processing IRQ
 *
 * @return True for IRQ, false otherwise.
 *
 */
int32_t platform_in_isr(void)
{
    return rt_interrupt_get_nest();
}

/**
 * platform_interrupt_enable
 *
 * Enable peripheral-related interrupt
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int32_t platform_interrupt_enable(uint32_t vector_id)
{
    RL_ASSERT(0 < disable_counter);

    platform_global_isr_disable();
    disable_counter--;

    if (disable_counter < 2)
    {
        rt_hw_interrupt_umask(vector_id);
    }

    platform_global_isr_enable();
    return 0;
}

/**
 * platform_interrupt_disable
 *
 * Disable peripheral-related interrupt.
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int32_t platform_interrupt_disable(uint32_t vector_id)
{
    RL_ASSERT(0 <= disable_counter);

    platform_global_isr_disable();
    /* virtqueues use the same NVIC vector
       if counter is set - the interrupts are disabled */
    if (disable_counter < 2)
    {
        rt_hw_interrupt_mask(vector_id);
    }

    disable_counter++;
    platform_global_isr_enable();
    return 0;
}

/**
 * platform_map_mem_region
 *
 * Dummy implementation
 *
 */
void platform_map_mem_region(uint32_t vrt_addr, uint32_t phy_addr, uint32_t size, uint32_t flags)
{
}

/**
 * platform_cache_all_flush_invalidate
 *
 * Dummy implementation
 *
 */
void platform_cache_all_flush_invalidate(void)
{
}

/**
 * platform_cache_disable
 *
 * Dummy implementation
 *
 */
void platform_cache_disable(void)
{
}

/**
 * platform_cache_flush
 *
 * Empty implementation
 *
 */
void platform_cache_flush(void *data, uint32_t len)
{
}

/**
 * platform_cache_invalidate
 *
 * Empty implementation
 *
 */
void platform_cache_invalidate(void *data, uint32_t len)
{
}

/**
 * platform_vatopa
 *
 * Dummy implementation
 *
 */
uintptr_t platform_vatopa(void *addr)
{
    return ((uintptr_t)(char *)addr);
}

/**
 * platform_patova
 *
 * Dummy implementation
 *
 */
void *platform_patova(uintptr_t addr)
{
    return ((void *)(char *)addr);
}

/**
 * platform_init
 *
 * platform/environment init
 */
int32_t platform_init(void)
{
    /* Create lock used in multi-instanced RPMsg */
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
    if (0 != env_create_mutex(&platform_lock, 1, &platform_lock_static_ctxt))
#else
    if (0 != env_create_mutex(&platform_lock, 1))
#endif
    {
        return -1;
    }

    return 0;
}

/**
 * platform_deinit
 *
 * platform/environment deinit process
 */
int32_t platform_deinit(void)
{
    /* Delete lock used in multi-instanced RPMsg */
    env_delete_mutex(platform_lock);
    platform_lock = ((void *)0);
    return 0;
}

// static void ipi_isr_handle(int irqn, void *param)
// {
//     rt_interrupt_enter();
//     rt_kprintf("ipi_isr_handle\n");
//     rt_uint32_t cpu_id = rt_hw_cpu_id();
//     rt_kprintf("irqn   %d\n", irqn);
//     rt_kprintf("cpu_id %d\n", cpu_id);
//     rt_interrupt_leave();
// }

// static void ipi_test_init(void)
// {
//     rt_hw_interrupt_install(0, ipi_isr_handle, RT_NULL, "ipi-0");
//     rt_hw_interrupt_install(1, ipi_isr_handle,  RT_NULL, "ipi-1");
// }
// MSH_CMD_EXPORT(ipi_test_init, ipi_test_init);

// static void ipi_interrupt_test(void)
// {
//     rt_uint32_t cpu_id = rt_hw_cpu_id();

//     if (cpu_id == 0)
//     {
//         rt_hw_ipi_send(0, (RT_CPU_MASK ^ (1 << 0)));
//         rt_thread_mdelay(1);
//         rt_hw_ipi_send(1, (RT_CPU_MASK ^ (1 << 0)));
//     }
//     else
//     {
//         rt_hw_ipi_send(0, (RT_CPU_MASK ^ (1 << 1)));
//         rt_thread_mdelay(1);
//         rt_hw_ipi_send(1, (RT_CPU_MASK ^ (1 << 1)));
//     }
// }
// MSH_CMD_EXPORT(ipi_interrupt_test, ipi_interrupt_test);
