/*
* Copyright (c) 2006-2021, RT-Thread Development Team
*
* SPDX-License-Identifier: Apache-2.0
*/
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <rpmsg_lite.h>
#include <rpmsg_queue.h>
#include <rpmsg_ns.h>

#define LOG_TAG "rpmsg master"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

/* CPU0 as master and CPU3 as remote. */
#define MASTER_CPU_ID       ((rt_uint32_t)0)
#define MASTER_EPT_ID       0x1005U
#define MASTER_EPT_NAME     "rpmsg_echo_dev_m"

#define REMOTE_CPU_ID       ((rt_uint32_t)1)
#define REMOTE_EPT_ID       0x3003U
#define REMOTE_EPT_NAME     "rpmsg_echo_dev_r"

/* 共享内存地址配置 */
#define RPMSG_SHMEM_BASE    AMP_DDR_RPMSG_START
#define RPMSG_SHMEM_END     AMP_DDR_RPMSG_END
#define RPMSG_SHMEM_SIZE    (AMP_DDR_RPMSG_END - AMP_DDR_RPMSG_START)

/* 回显设备 */
struct rpmsg_echo_dev
{
    struct rpmsg_lite_instance *instance;
    struct rpmsg_lite_endpoint *ept;
    rpmsg_queue_handle queue;
    rpmsg_ns_handle name_service;
    unsigned long remote_addr;
};
typedef struct rpmsg_echo_dev rpmsg_echo_dev_t;

static rpmsg_echo_dev_t echo_device = {0};
static rt_bool_t is_init = RT_FALSE;

/* 检查共享内存是否足够 */
static void rpmsg_share_mem_check(void)
{
    if ((RPMSG_SHMEM_BASE + RPMSG_SHMEM_SIZE) > RPMSG_SHMEM_END)
    {
        LOG_E("share memory size error!");
        while (1);
    }
}

static void app_nameservice_isr_cb(uint32_t new_ept, const char *new_ept_name, uint32_t flags, void *user_data)
{
    unsigned long *data = (unsigned long *)user_data;
    *data = new_ept;
    LOG_I("new_ept = 0x%x, new_ept_name = %s, flags = 0x%x, user_data = %p", new_ept, new_ept_name, flags, user_data);
}

static void rpmsg_echo_master_init(void)
{
    rpmsg_echo_dev_t *device = &echo_device;
    rt_uint32_t master_id = MASTER_CPU_ID;
    rt_uint32_t remote_id = REMOTE_CPU_ID;

    LOG_I("master core cpu_id-%ld", master_id);
    LOG_I("shmem 0x%lx - 0x%lx", RPMSG_SHMEM_BASE, RPMSG_SHMEM_END);

    /* 实例化主机 link_id = 0 .. 3 */
    device->instance = rpmsg_lite_master_init((void *)RPMSG_SHMEM_BASE, RPMSG_SHMEM_SIZE, 0, RL_NO_FLAGS);
    if (!device->instance)
    {
        LOG_E("rpmsg_lite_master_init err.");
    }

    device->queue = rpmsg_queue_create(device->instance);
    if (!device->queue)
    {
        LOG_E("rpmsg_queue_create err.");
    }

    device->ept = rpmsg_lite_create_ept(device->instance, MASTER_EPT_ID, rpmsg_queue_rx_cb, device->queue);
    if (!device->ept)
    {
        LOG_E("rpmsg_lite_create_ept err.");
    }

    device->name_service = rpmsg_ns_bind(device->instance, app_nameservice_isr_cb, (void *)&device->remote_addr);
    if (!device->name_service)
    {
        LOG_E("rpmsg_ns_bind err.");
    }

    /* 等待从机连接 */
    while (device->remote_addr == 0)
    {
        rt_thread_mdelay(1);
    }
}

static void rpmsg_echo_master_deinit(void)
{
    rpmsg_echo_dev_t *device = &echo_device;
    rpmsg_lite_destroy_ept(device->instance, device->ept);
    rpmsg_queue_destroy(device->instance, device->queue);
    rpmsg_ns_unbind(device->instance, device->name_service);
    rpmsg_lite_deinit(device->instance);
    LOG_I("RPMsg-Lite echo demo ends");
}

static void rpmsg_echo_master_send(const char *msg)
{
    rpmsg_echo_dev_t *device = &echo_device;
    rt_int32_t ret = RL_SUCCESS;
    rt_uint32_t remote_ept_id = 0;
    rt_uint32_t r_len = 0;
    char *buffer = (char *)rt_calloc(1, RL_BUFFER_PAYLOAD_SIZE);

    ret = rpmsg_lite_send(device->instance, device->ept, device->remote_addr, (char *)msg, rt_strlen(msg), RL_BLOCK);
    if (ret != RL_SUCCESS)
    {
        LOG_E("rpmsg_lite_send error, ret is 0x%x", ret);
        rt_free(buffer);
        return;
    }

    ret = rpmsg_queue_recv(device->instance, device->queue, (uint32_t *)&remote_ept_id,
        buffer, RL_BUFFER_PAYLOAD_SIZE, &r_len, RL_BLOCK);
    if (ret != RL_SUCCESS)
    {
        LOG_E("rpmsg_queue_recv error, ret is 0x%x", ret);
        rt_free(buffer);
        return;
    }

    LOG_I("remote_ept_id = 0x%x, len = %d, msg = %s", remote_ept_id, r_len, buffer);

    rt_free(buffer);
    buffer = RT_NULL;
}

/* rpmsg_echo_master init
 * rpmsg_echo_master deinit
 * rpmsg_echo_master send xxxx
 */
static int rpmsg_echo_master(int argc, char **argv)
{
    LOG_I("RPMsg-Lite echo master v1.0");
    LOG_I("Current cpu_id is %d", rt_hw_cpu_id());

    /* 检查运行 CPU 与配置是否一致 */
    RT_ASSERT(rt_hw_cpu_id() == MASTER_CPU_ID);
    if (argc == 2)
    {
        if (!rt_strcmp(argv[1], "init"))
        {
            if (is_init == RT_FALSE)
            {
                is_init = RT_TRUE;
                rpmsg_share_mem_check();
                rpmsg_echo_master_init();
            }
        }
        else if (!rt_strcmp(argv[1], "deinit"))
        {
            if (is_init == RT_TRUE)
            {
                rpmsg_echo_master_deinit();
                is_init = RT_FALSE;
            }
        }
        else
        {
            goto _exit;
        }
    }
    else if (argc == 3)
    {
        if (!rt_strcmp(argv[1], "send"))
        {
            if (is_init == RT_TRUE)
            {
                rpmsg_echo_master_send(argv[2]);
            }
        }
        else
        {
            goto _exit;
        }
    }
    else
    {
        goto _exit;
    }

    return RT_EOK;
_exit:
    LOG_I("rpmsg_echo_master init");
    LOG_I("rpmsg_echo_master deinit");
    LOG_I("rpmsg_echo_master send <string>");

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(rpmsg_echo_master, rpmsg_echo_master, The rpmsg-lite echo master demo.);
