/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#ifndef  CPUPORT_H__
#define  CPUPORT_H__

#include <rtdef.h>

rt_inline void rt_hw_isb(void)
{
    __asm volatile ("isb":::"memory");
}

rt_inline void rt_hw_dmb(void)
{
    __asm volatile ("dmb":::"memory");
}

rt_inline void rt_hw_dsb(void)
{
    __asm volatile ("dsb":::"memory");
}

#endif  /*CPUPORT_H__*/
