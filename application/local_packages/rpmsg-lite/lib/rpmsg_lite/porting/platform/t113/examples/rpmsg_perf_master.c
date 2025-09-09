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
#include <stdlib.h>
#include "hwtimer.h"

#define LOG_TAG "rpmsg master"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

/* CPU0 as master and CPU3 as remote. */
#define MASTER_CPU_ID           ((rt_uint32_t)0)
#define MASTER_EPT_ID           0x1015U
#define MASTER_EPT_NAME         "rpmsg_perf_dev_m"

#define REMOTE_CPU_ID           ((rt_uint32_t)3)
#define REMOTE_EPT_ID           0x3013U
#define REMOTE_EPT_NAME         "rpmsg_perf_dev_r"

/* 共享内存地址配置 */
#define RPMSG_SHMEM_BASE        AMP_DDR_RPMSG_START
#define RPMSG_SHMEM_END         AMP_DDR_RPMSG_END
#define RPMSG_SHMEM_SIZE        (2UL * RL_VRING_OVERHEAD)

static struct rk3568_hwtimer time5 =
{
    .base                   = TIMER5_MMIO_BASE,
    .irq                    = TIMER5_IRQ,
    .int_flag               = RT_FALSE,
};
/* 回显设备 */
struct rpmsg_echo_dev
{
    struct rpmsg_lite_instance  *instance;
    struct rpmsg_lite_endpoint  *ept;
    rpmsg_queue_handle          queue;
    rpmsg_ns_handle             name_service;
    unsigned long               remote_addr;
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

static void rpmsg_perf_master_init(void)
{
    rpmsg_echo_dev_t *device = &echo_device;
    rt_uint32_t master_id = MASTER_CPU_ID;
    rt_uint32_t remote_id = REMOTE_CPU_ID;

    LOG_I("master core cpu_id-%ld", master_id);
    LOG_I("shmem 0x%lx - 0x%lx", RPMSG_SHMEM_BASE, RPMSG_SHMEM_END);

    /* 实例化主机 link_id = 0 .. 3 */
    device->instance = rpmsg_lite_master_init((void *)RPMSG_SHMEM_BASE, RPMSG_SHMEM_SIZE, 0, RL_NO_FLAGS);

    device->queue = rpmsg_queue_create(device->instance);
    device->ept = rpmsg_lite_create_ept(device->instance, MASTER_EPT_ID, rpmsg_queue_rx_cb, device->queue);
    device->name_service = rpmsg_ns_bind(device->instance, app_nameservice_isr_cb, (void *)&device->remote_addr);

    /* 等待从机连接 */
    while (device->remote_addr == 0)
    {
        rt_thread_mdelay(1);
    }

    rk3568_timer_init(&time5);
}

static void rpmsg_perf_master_deinit(void)
{
    rpmsg_echo_dev_t *device = &echo_device;
    rpmsg_lite_destroy_ept(device->instance, device->ept);
    rpmsg_queue_destroy(device->instance, device->queue);
    rpmsg_ns_unbind(device->instance, device->name_service);
    rpmsg_lite_deinit(device->instance);
    LOG_I("RPMsg-Lite echo demo ends");
}

static void rpmsg_perf_master_send(void)
{
    rpmsg_echo_dev_t *device = &echo_device;
    rt_int32_t ret = RL_SUCCESS;
    rt_uint32_t remote_ept_id = 0;
    rt_uint32_t r_len = 0;
    rt_uint32_t echo_tick = 0;
    float total_time = 0;
    rt_uint8_t tx_buf[RL_BUFFER_PAYLOAD_SIZE] = {0};
    float speed = 0.0f;
    char *buffer = (char *)rt_calloc(1, RL_BUFFER_PAYLOAD_SIZE);

    for(rt_uint32_t i = 0; i < RL_BUFFER_PAYLOAD_SIZE; i++)
    {
        tx_buf[i] = rand() % 100;
    }

    time5.ops->hwtimer_start(&time5);
    ret = rpmsg_lite_send(device->instance, device->ept, device->remote_addr, (char *)tx_buf, RL_BUFFER_PAYLOAD_SIZE, RL_BLOCK);
    if (ret != RL_SUCCESS)
    {
        LOG_E("rpmsg_lite_send error, ret is 0x%x", ret);
        rt_free(buffer);
        return;
    }

    ret = rpmsg_queue_recv(device->instance, device->queue, (uint32_t *)&remote_ept_id,
        buffer, RL_BUFFER_PAYLOAD_SIZE, &r_len, RL_BLOCK);

    echo_tick = time5.ops->hwtimer_get(&time5);
    time5.ops->hwtimer_stop(&time5);

    if (ret != RL_SUCCESS)
    {
        LOG_E("rpmsg_queue_recv error, ret is 0x%x", ret);
        rt_free(buffer);
        return;
    }

    for(rt_uint32_t i = 0; i < RL_BUFFER_PAYLOAD_SIZE; i++)
    {
        if(tx_buf[i] != buffer[i])
        {
            LOG_E("error in data len%d, tx:%d!=rx%d", i, tx_buf[i], buffer[i]);
            return;
        }
    }

    total_time = echo_tick / 48000000.0f;
    speed = ((((RL_BUFFER_PAYLOAD_SIZE + 16) / (total_time))) / 1048576.0f);

    LOG_I("%d Bytes, time:%dus, speed:%d.%04dMBytes/s", (r_len + 16), (rt_int32_t)(total_time * 1000000),
            (rt_int32_t)speed,(rt_int32_t)((speed - (rt_int32_t)speed) * 10000));

    rt_free(buffer);
    buffer = RT_NULL;
}

static int rpmsg_perf_master(int argc, char **argv)
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
                rpmsg_perf_master_init();
            }
        }
        else if (!rt_strcmp(argv[1], "deinit"))
        {
            if (is_init == RT_TRUE)
            {
                rpmsg_perf_master_deinit();
                is_init = RT_FALSE;
            }
        }
        else if (!rt_strcmp(argv[1], "perf"))
        {
            if (is_init == RT_TRUE)
            {
                rpmsg_perf_master_send();
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
    LOG_I("rpmsg_perf_master init");
    LOG_I("rpmsg_perf_master deinit");
    LOG_I("rpmsg_perf_master perf");

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(rpmsg_perf_master, rpmsg_perf_master, The rpmsg-lite perf master demo.);
