/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020/12/31     Bernard      Add license info
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtthread.h>

#define readb(reg)                          (*((volatile unsigned char *)(reg)))
#define readw(reg)                          (*((volatile unsigned short *)(reg)))
#define readl(reg)                          (*((volatile unsigned int *)(reg)))
#define readq(reg)                          (*((volatile unsigned long long *)(reg)))

#define writeb(data, reg)                   ((*((volatile unsigned char *)(reg))) = (unsigned char)(data))
#define writew(data, reg)                   ((*((volatile unsigned short *)(reg))) = (unsigned short)(data))
#define writel(data, reg)                   ((*((volatile unsigned int *)(reg))) = (unsigned int)(data))
#define writeq(data, reg)                   ((*((volatile unsigned long long *)(reg))) = (unsigned long long)(data))

#define PC_CFG0_ADDR 0x02000060
#define PC_DAT_ADDR  0x02000070

int main2(void)
{
    int count = 0;

    while (1)
    {
        printf("Thread 2 #%d\n", count++);
        rt_thread_yield();
    }

    return 0;
}
// INIT_APP_EXPORT(main2);

int main3(void)
{
    int count = 0;

    while (1)
    {
        printf("Thread 3 #%d\n", count++);
        rt_thread_yield();
    }

    return 0;
}
// INIT_APP_EXPORT(main3);

int main(void)
{
    unsigned int reg_val = 0;

    printf("Hello RT-Thread! [%s %s]\n", __DATE__, __TIME__);

    reg_val = readl(PC_CFG0_ADDR);
    reg_val &= ~(0xff);
    reg_val |= (0x11);
    writel(reg_val, PC_CFG0_ADDR);

    reg_val = readl(PC_DAT_ADDR);
    reg_val |= 0x3;
    writel(reg_val, PC_DAT_ADDR);

    return 0;
}

