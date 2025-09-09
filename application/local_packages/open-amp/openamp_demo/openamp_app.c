#include "openamp_app.h"

#ifndef AMP_SHARE_MEM_PHYSICS_ADDRESS
#define AMP_SHARE_MEM_PHYSICS_ADDRESS 0x8000000099000000
#endif

#ifndef AMP_SHARE_MEMORY_SIZE
#define AMP_SHARE_MEMORY_SIZE 0x10000
#endif

#ifndef AMP_VDEV_STATUS_SIZE
#define AMP_VDEV_STATUS_SIZE 0x1000
#endif

#define AMP_VDEV_ALIGN_SIZE 0x1000

#ifndef AMP_DEMO_VRING_TX_DES_NUM
#define AMP_DEMO_VRING_TX_DES_NUM 16
#endif

#ifndef AMP_DEMO_VRING_RX_DES_NUM
#define AMP_DEMO_VRING_RX_DES_NUM 16
#endif

#ifndef AMP_REMOTE_CPU_ID
#define AMP_REMOTE_CPU_ID 1
#endif

#ifndef AMP_REMOTE_ENTRY
#define AMP_REMOTE_ENTRY 0x97000000
#endif

#ifndef AMP_CPU_SGI_INTERRUPT_ID
#define AMP_SLAVE_IRQ_ID 7
#endif

static int app_auto_node_register(const char *name)
{
    openamp_app_node_t app_node = NULL;

    if(name == NULL)
    {
        return -1;
    }

    if(!strncmp(name, ECHO_NAME, APP_NODE_MAXNUM))
    {
        app_echo_init();
        rt_kprintf("app_auto_node_register echo node\n");
    }

    return 0;
}

static struct openamp_virtio_device app = {
#ifdef AMP_AUTO_MEMORY
    .vdev_status_size = AMP_VDEV_STATUS_SIZE,
    .align_size       = AMP_VDEV_ALIGN_SIZE,
#else
    .vring_buf  = AMP_DEMO_VRING_BUF_ADDR,
    .vring_size = AMP_DEMO_VRING_SIZE,
    .vring_tx   = AMP_DEMO_VRING_TX_ADDR,
    .vring_rx   = AMP_DEMO_VRING_RX_ADDR,
#endif
    .tx_num                  = AMP_DEMO_VRING_TX_DES_NUM,
    .rx_num                  = AMP_DEMO_VRING_RX_DES_NUM,
    .shm_physaddr            = (void *)(AMP_SHARE_MEM_PHYSICS_ADDRESS),
    .shm_vaddr               = (void *)(AMP_SHARE_MEM_PHYSICS_ADDRESS),
    .shm_size                = AMP_SHARE_MEMORY_SIZE,
    .secondary_config.cpu_id = AMP_REMOTE_CPU_ID,
    .secondary_config.entry  = AMP_REMOTE_ENTRY,
#if defined(AMP_USING_IPI)
    .local_sgi_id  = AMP_CPU_SGI_INTERRUPT_ID,
    .remote_sgi_id = AMP_CPU_SGI_INTERRUPT_ID,
#elif defined(AMP_USING_HW_MAILBOX)
    .local_sgi_id  = AMP_HW_LOCAL_MAILBOX_CHANNEL,
    .remote_sgi_id = AMP_HW_REMOTE_MAILBOX_CHANNEL,
#endif
    .auto_node_register = app_auto_node_register,
};

static int openamp_app_init(void)
{
    amp_sigiface_dev_create(&app.sigiface_ops);
    if(app.sigiface_ops == NULL)
    {
        printf("openamp sigiface ops is NULL\n");
        return -1;
    }

    openamp_dev_create(&app);

    return 0;
}

#ifdef USER_RT_THREAD_RTOS
INIT_ENV_EXPORT(openamp_app_init);
#endif
