#ifndef __PLATFORM_PORT_H__
#define __PLATFORM_PORT_H__

// #include <arch/addrspace.h>
// #include <cpu.h>
#include <rtthread.h>

#ifdef RT_AMP_MASTER
#define AMP_MASTER
#define VIRTIO_DRIVER_ONLY
#elif defined RT_AMP_SLAVE
#define AMP_SLAVE
#define VIRTIO_DEVICE_ONLY
#endif

#ifndef RPMSG_BUFFER_SIZE
#define RPMSG_BUFFER_SIZE (512)
#endif

#define OPENAMP_APP_NODE_NAME_MAXLEN 20

#define APP_NODE_MAXNUM              16

#define TIMESTAMP_GET                rt_tick_get()
#define SLEEP_USEC(x)                rt_thread_mdelay(x / 1000)
#define FREE                         rt_free
#define MALLOC                       rt_malloc
#define INTERRUPT_ENABLE(flags)      rt_hw_interrupt_enable(flags)
#define INTERRUPT_DISABLE            rt_hw_interrupt_disable()
#define MUTEEX_INIT(mutex)           rt_mutex_init(mutex, "m", RT_IPC_FLAG_PRIO)
#define MUTEEX_DEINIT(mutex)         rt_mutex_detach(mutex)
#define MUTEEX_TAKE(mutex, timeout)  rt_mutex_take(mutex, timeout)
#define MUTEEX_RELEASE(mutex)        rt_mutex_release(mutex)
#define MUTEEX_TRYTAKE(mutex)        rt_mutex_trytake(mutex)
#define MUTEEX_GET_OWNER(mutex)      rt_mutex_get_owner(mutex)
#define HW_CACHE_FLUSH(addr, len) \
    do                            \
    {                             \
    } while(0)
#define HW_CACHE_INVALIDATE(addr, len) \
    do                                 \
    {                                  \
    } while(0)

typedef struct rt_mutex metal_mutex_t;

#if defined(AMP_MASTER)
int pull_up_cpu(rt_uint64_t entry);
#endif

#endif /* __PLATFORM_PORT_H__ */
