/*
 * COPYRIGHT (C) 2012-2020, Shanghai Real-Thread Technology Co., Ltd
 * All rights reserved.
 * Change Logs:
 * Date           Author       Notes
 * 2018-02-08     RT-Thread    the first version
 */


#ifndef __DRV_UART_H__
#define __DRV_UART_H__

#include <rtthread.h>

#define UART0_BASE_ADDR 0x02500000
#define UART1_BASE_ADDR 0x02500400
#define UART2_BASE_ADDR 0x02500800
#define UART3_BASE_ADDR 0x02500C00
#define UART4_BASE_ADDR 0x02501000
#define UART5_BASE_ADDR 0x02501400

#define UART0_IRQ       34
#define UART1_IRQ       35
#define UART2_IRQ       36
#define UART3_IRQ       37
#define UART4_IRQ       38
#define UART5_IRQ       39

#define REG_UART_RBR    0x0000
#define REG_UART_THR    0x0000
#define REG_UART_DLL    0x0000
#define REG_UART_DLH    0x0004
#define REG_UART_IER    0x0004
#define REG_UART_IIR    0x0008
#define REG_UART_FCR    0x0008
#define REG_UART_LCR    0x000c
#define REG_UART_MCR    0x0010
#define REG_UART_LSR    0x0014
#define REG_UART_MSR    0x0018
#define REG_UART_SCH    0x001c
#define REG_UART_USR    0x007c
#define REG_UART_TFL    0x0080
#define REG_UART_RFL    0x0084
#define REG_UART_HSK    0x0088
#define REG_UART_HALT   0X00A4

int rt_hw_uart_init(void);

#endif /* __DRV_UART_H__ */
