
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

#define LOG_TAG "rpmsg remote"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

/* CPU0 as master and CPU3 as remote. */
#define MASTER_CPU_ID       ((rt_uint32_t)0)
#define MASTER_EPT_ID       0x1005U
#define MASTER_EPT_NAME     "rpmsg_echo_dev_m"

#define REMOTE_CPU_ID       ((rt_uint32_t)1)
#define REMOTE_EPT_ID       0x3003U
#define REMOTE_EPT_NAME     "rpmsg_chrdev"

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
    LOG_I("new_ept = 0x%x, new_ept_name = %s, flags = 0x%x, user_data = %p", new_ept, new_ept_name, flags, user_data);
}

static void rpmsg_echo_remote_thread_entry(void *p)
{
    rpmsg_echo_dev_t *device = (rpmsg_echo_dev_t *)p;
    rt_int32_t ret = RL_SUCCESS;
    rt_uint32_t master_ept_id = 0;
    rt_uint32_t r_len = 0;
    char *buffer = (char *)rt_calloc(10, RL_BUFFER_PAYLOAD_SIZE);
    rt_kprintf("r_len addr 0x%x\n", &r_len);
    while (1)
    {
        ret = rpmsg_queue_recv(device->instance, device->queue, (uint32_t *)&master_ept_id,
            buffer, RL_BUFFER_PAYLOAD_SIZE, &r_len, RL_BLOCK);
        if (ret != RL_SUCCESS)
        {
            LOG_E("rpmsg_queue_recv error, ret is 0x%x", ret);
            break;
        }

        rt_kprintf("master_ept_id: %X, r_len: %d   %s\n", master_ept_id, r_len,buffer);

        ret = rpmsg_lite_send(device->instance, device->ept, master_ept_id, buffer, r_len, RL_BLOCK);
        if (ret != RL_SUCCESS)
        {
            LOG_E("rpmsg_lite_send error, ret is 0x%x", ret);
            break;
        }
    }

    rt_free(buffer);
    buffer = RT_NULL;
    rpmsg_lite_destroy_ept(device->instance, device->ept);
    rpmsg_queue_destroy(device->instance, device->queue);
    rpmsg_ns_unbind(device->instance, device->name_service);
    rpmsg_lite_deinit(device->instance);
}

static void rpmsg_echo_remote_init(void)
{
    rpmsg_echo_dev_t *device = &echo_device;
    rt_uint32_t master_id = MASTER_CPU_ID;
    rt_uint32_t remote_id = REMOTE_CPU_ID;
    uint32_t ret = RL_TRUE;
    rt_thread_t tid = RT_NULL;

    LOG_I("remote core cpu_id-%ld", remote_id);
    LOG_I("shmem 0x%lx - 0x%lx", RPMSG_SHMEM_BASE, RPMSG_SHMEM_END);

    /* 实例化主机 link_id = 0 .. 3 */
    device->instance = rpmsg_lite_remote_init((void *)RPMSG_SHMEM_BASE, 0, RL_NO_FLAGS);
    if (!device->instance)
    {
        LOG_E("rpmsg_lite_remote_init err.");
    }

    /* 等待连接成功 */
    ret = rpmsg_lite_wait_for_link_up(device->instance, (uint32_t)RL_BLOCK);
    if (ret == RL_TRUE)
    {
        LOG_I("link up! link_id is 0x%lx", device->instance->link_id);
    }
    else
    {
        LOG_E("link timeout! link_id is 0x%lx", device->instance->link_id);
        ASSERT(0);
    }

    device->queue = rpmsg_queue_create(device->instance);
    if (!device->queue)
    {
        LOG_E("rpmsg_queue_create err.");
    }

    device->ept = rpmsg_lite_create_ept(device->instance, REMOTE_EPT_ID, rpmsg_queue_rx_cb, device->queue);
    if (!device->ept)
    {
        LOG_E("rpmsg_lite_create_ept err.");
    }

    device->name_service = rpmsg_ns_bind(device->instance, app_nameservice_isr_cb, RT_NULL);
    if (!device->name_service)
    {
        LOG_E("rpmsg_ns_bind err.");
    }

    rpmsg_ns_announce(device->instance, device->ept, REMOTE_EPT_NAME, RL_NS_CREATE);

    tid = rt_thread_create("rpmsg_echo_remote", rpmsg_echo_remote_thread_entry, device, 40960,
        RT_MAIN_THREAD_PRIORITY - 1, 20);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);

    LOG_I("RPMsg-Lite echo demo ends");
}

static int rpmsg_echo_remote(void)
{
    LOG_I("RPMsg-Lite echo remote v1.0");
    LOG_I("Current cpu_id is %d", rt_hw_cpu_id());

    /* 检查运行 CPU 与配置是否一致 */
    RT_ASSERT(rt_hw_cpu_id() == REMOTE_CPU_ID);

    if (is_init == RT_FALSE)
    {
        is_init = RT_TRUE;
        rpmsg_share_mem_check();
        rpmsg_echo_remote_init();
    }
    else
    {
        LOG_W("The rpmsg_echo_remote is running");
    }

    return RT_EOK;
}
// INIT_APP_EXPORT(rpmsg_echo_remote);
MSH_CMD_EXPORT_ALIAS(rpmsg_echo_remote, rpmsg_echo_remote, RPMsg-Lite echo remote demo.);

static void hexdump(unsigned int start_base, unsigned int len)
{
    unsigned int i = 0, j=  0;
    unsigned char val = 0;
    char ascii_buf[17] = {0};

    rt_kprintf("Dump 0x%p %dBytes\r\n", start_base, len);
    rt_kprintf("Offset      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");

    for (i = 0; i < len; i += 16)
    {
        rt_kprintf("0x%08X  ", start_base + i);

        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                val = *((unsigned char *)(start_base + i + j));
                rt_kprintf("%02X ", val);
                ascii_buf[j] = (val >= 32 && val <= 126) ? val : '.';
            }
            else
            {
                rt_kprintf("   ");
                ascii_buf[j] = ' ';
            }
        }

        rt_kprintf(" %s\r\n", ascii_buf);
    }
}
#include <armv7.h>
void dump_echo_remote_stack(void)
{
    rt_thread_t tid = RT_NULL;
    int i = 0;
    struct rt_hw_stack *stack = RT_NULL;

    tid = rt_thread_find("rpmsg_echo_remote");
    if (tid != RT_NULL)
    {
        rt_kprintf("%s stack info : \n", tid->name);
        rt_kprintf("stack_addr : 0x%x\n", tid->stack_addr);
        rt_kprintf("sp         : 0x%x\n", tid->sp);
        // hexdump(tid->stack_addr, tid->stack_size);
        stack = tid->sp;
        rt_kprintf("cpsr : 0x%08x\n", stack->cpsr);
        rt_kprintf("r0   : 0x%08x\n", stack->r0);
        rt_kprintf("r1   : 0x%08x\n", stack->r1);
        rt_kprintf("r2   : 0x%08x\n", stack->r2);
        rt_kprintf("r3   : 0x%08x\n", stack->r3);
        rt_kprintf("r4   : 0x%08x\n", stack->r4);
        rt_kprintf("r5   : 0x%08x\n", stack->r5);
        rt_kprintf("r6   : 0x%08x\n", stack->r6);
        rt_kprintf("r7   : 0x%08x\n", stack->r7);
        rt_kprintf("r8   : 0x%08x\n", stack->r8);
        rt_kprintf("r9   : 0x%08x\n", stack->r9);
        rt_kprintf("r10  : 0x%08x\n", stack->r10);
        rt_kprintf("fp   : 0x%08x\n", stack->fp);
        rt_kprintf("ip   : 0x%08x\n", stack->ip);
        rt_kprintf("lr   : 0x%08x\n", stack->lr);
        rt_kprintf("pc   : 0x%08x\n", stack->pc);
    }
}
MSH_CMD_EXPORT(dump_echo_remote_stack, dump_echo_remote_stack);
