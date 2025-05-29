#ifndef __DRV_UART_H__
#define __DRV_UART_H__

#include "board.h"
#include "drv_iomux.h"

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

#define UART_SOFT_FIFO_SIZE    512

enum uart_id
{
    UART0 = 0,
    UART1 = 1,
    UART2 = 2,
    UART3 = 3,
    UART4 = 4,
    UART5 = 5,
};

enum uart_baud_rate
{
    BAUD_RATE_2400    = 2400,
    BAUD_RATE_4800    = 4800,
    BAUD_RATE_9600    = 9600,
    BAUD_RATE_19200   = 19200,
    BAUD_RATE_38400   = 38400,
    BAUD_RATE_57600   = 57600,
    BAUD_RATE_115200  = 115200,
    BAUD_RATE_230400  = 230400,
    BAUD_RATE_460800  = 460800,
    BAUD_RATE_921600  = 921600,
};

enum uart_data_bits
{
    UART_DATA_BITS_5 = 5,
    UART_DATA_BITS_6 = 6,
    UART_DATA_BITS_7 = 7,
    UART_DATA_BITS_8 = 8,
};

enum uart_stop_bits
{
    UART_STOP_BITS_1   = 1,
    UART_STOP_BITS_1_5 = 2,
};

enum uart_parity
{
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD  = 1,
    UART_PARITY_EVEN = 2,
};

struct uart_configure
{
    enum uart_baud_rate baud_rate;
    enum uart_data_bits data_bits;
    enum uart_stop_bits stop_bits;
    enum uart_parity    parity;
};

struct rt_uart_rx_fifo
{
    char         buffer[UART_SOFT_FIFO_SIZE];
    unsigned int put_index;
    unsigned int get_index;
    int          is_full;
};

struct uart_handle
{
    /* user define */
    unsigned int          base;
    unsigned int          irq;
    enum uart_id          id;
    struct uart_configure cfg;
    struct iomux_cfg      gpio_tx;
    struct iomux_cfg      gpio_rx;

    /* private */
    struct rt_uart_rx_fifo fifo;
};

int uart_init(struct uart_handle *uart);
int uart_deinit(struct uart_handle *uart);
int uart_putc(struct uart_handle *uart, char c);
char uart_getc(struct uart_handle *uart);
int uart_bind_recv_callback(void (*callback)(void *param));

#endif /* __DRV_UART_H__ */
