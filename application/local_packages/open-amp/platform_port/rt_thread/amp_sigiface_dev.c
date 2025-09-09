#include "amp_sigiface_dev.h"
#include "amp_mailbox.h"

#define AMP_THREAD_NAME       "amp_thread"
#define AMP_THREAD_STACK_SIZE 8192
#define AMP_THREAD_PRIORITY   5
#define AMP_THREAD_TICK       10

#ifdef AMP_USING_HW_MAILBOX
#define AMP_MBOX_DEV_NAME "amp_mbox"
static rt_device_t mbox_dev = RT_NULL;
#endif

static rt_sem_t sigiface_sem = NULL;

static void sigiface_callback(int vector, void *param)
{
    rt_sem_t sem = (rt_sem_t)param;

    /* signal the semaphore */
    rt_sem_release(sem);
}

static void sigiface_thread_entry(void *parameter)
{
    openamp_virtio_device_t dev = (openamp_virtio_device_t)parameter;
    struct virtqueue       *rx_vq;

    rx_vq = openamp_get_vq(dev);

    while(1)
    {
        rt_sem_take(sigiface_sem, RT_WAITING_FOREVER);

        /* openamp的virtual queue处理, 在任务上下午调用endpoint cb */
        openamp_virqueue_notification(rx_vq);
    }
}

static int amp_sigiface_init(void *dev)
{
    rt_thread_t             tid;
    openamp_virtio_device_t opdev = (openamp_virtio_device_t)dev;

    if(dev == RT_NULL)
    {
        rt_kprintf("openamp device is NULL\n");
        return -1;
    }

    sigiface_sem = rt_sem_create("sigiface_sem", 0, RT_IPC_FLAG_FIFO);
    if(sigiface_sem == RT_NULL)
    {
        rt_kprintf("openamp sigiface sem create failed!\r\n");
        return -RT_ENOMEM;
    }

    /* install the callback */
#if defined(AMP_USING_IPI)
    // rt_hw_ipi_install(opdev->local_sgi_id, sigiface_sem, sigiface_callback);
    rt_hw_interrupt_install(opdev->local_sgi_id, sigiface_sem, sigiface_callback, "ipi");
#elif defined(AMP_USING_HW_MAILBOX)
    struct mbox_configure cfg = {0};

    mbox_dev = rt_device_find(AMP_MBOX_DEV_NAME);
    if(mbox_dev == RT_NULL)
    {
        LOG_E("%s device not found", AMP_MBOX_DEV_NAME);
        return -1;
    }

    cfg.channel       = dev->local_sgi_id;
    cfg.mbox_callback = sigiface_callback;
    cfg.param         = sigiface_sem;
    cfg.init_msg.cmd  = MBOX_RPMSG_MAGIC0;
    cfg.init_msg.data = MBOX_RPMSG_MAGIC1;

    rt_device_open(mbox_dev, RT_DEVICE_OFLAG_RDWR);
    rt_device_control(mbox_dev, RT_DEVICE_CTRL_CONFIG, (void *)&cfg);
#endif

    tid = rt_thread_create(AMP_THREAD_NAME, sigiface_thread_entry, opdev, AMP_THREAD_STACK_SIZE, AMP_THREAD_PRIORITY, AMP_THREAD_TICK);
    if(tid == RT_NULL)
    {
        rt_kprintf("openamp thread create failed!\r\n");
        return -RT_ERROR;
    }

    rt_thread_startup(tid);

#if defined(AMP_MASTER)
    pull_up_cpu(opdev->secondary_config.entry);
#endif

    return 0;
}

static int amp_sigiface_send(int sigiface_id, rt_uint8_t remote_cpu)
{
#if defined(AMP_USING_IPI)
    static rt_uint32_t cpu_mask;

    /* call SGI to generate notify */
    // rt_kprintf("send to CPU#%d openamp_notify ...SGI(%d)\r\n", remote_cpu, sigiface_id);

    // rt_hw_ipi_pending(remote_cpu, sigiface_id);
    rt_hw_ipi_send(sigiface_id, RT_CPU_MASK ^ (1 << remote_cpu));
#elif defined(AMP_USING_HW_MAILBOX)
    struct mbox_cmd_data msg = {0};

    msg.cmd  = MBOX_RPMSG_MAGIC0;
    msg.data = MBOX_RPMSG_MAGIC1 | (remote_cpu & 0xF);
    rt_device_write(mbox_dev, sigiface_id, &msg, sizeof(struct mbox_cmd_data));
#endif /* PKG_INTER_CORE_USING_IPI */
    return 0;
}

static struct amp_sigiface_ops amp_sigiface_ops = {
    .sigiface_init = amp_sigiface_init,
    .sigiface_send = amp_sigiface_send,
};

int amp_sigiface_dev_create(struct amp_sigiface_ops **sigiface_ops)
{
    *sigiface_ops = &amp_sigiface_ops;

    return 0;
}
