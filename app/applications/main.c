/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020/12/31     Bernard      Add license info
 */
#include <rtthread.h>
#include <board.h>

static void thread_entry1(void *parameter)
{
    while (1)
    {
        rt_kprintf("thread1 running! tick %d\n", rt_tick_get());
        rt_thread_mdelay(500); // Sleep for 1000 milliseconds
    }
}

static void thread_entry2(void *parameter)
{
    while (1)
    {
        rt_kprintf("thread2 running! tick %d\n", rt_tick_get());
        rt_thread_mdelay(600); // Sleep for 1000 milliseconds
    }
}

void smp_test(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("thread1", thread_entry1, RT_NULL, 1024, 10, 10);
    if (tid != RT_NULL)
    {
        rt_thread_control(tid, RT_THREAD_CTRL_BIND_CPU, (void *)0); // Bind to CPU 0
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("Failed to create thread\n");
    }

    tid = rt_thread_create("thread2", thread_entry2, RT_NULL, 1024, 10, 10);
    if (tid != RT_NULL)
    {
        rt_thread_control(tid, RT_THREAD_CTRL_BIND_CPU, (void *)1); // Bind to CPU 0
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("Failed to create thread\n");
    }
}
MSH_CMD_EXPORT(smp_test, smp test);

int main(void)
{
    rt_kprintf("Hello RT-Thread!\n");

    return 0;
}

