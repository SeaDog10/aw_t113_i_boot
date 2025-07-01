#include "ymodem_port.h"
#include "drv_uart.h"
#include "board.h"

// extern struct uart_handle uart0;
struct uart_handle uart1 =
{
    .base     = UART1_BASE_ADDR,
    .irq      = UART1_IRQ,
    .id       = UART1,
    .cfg      =
    {
       .baud_rate = BAUD_RATE_115200,
       .data_bits = UART_DATA_BITS_8,
       .stop_bits = UART_STOP_BITS_1,
       .parity    = UART_PARITY_NONE,
    },
    .gpio_tx =
    {
        .port = IO_PORTB,
        .pin  = PIN_8,
        .mux  = IO_PERIPH_MUX7,
        .pull = IO_PULL_RESERVE,
    },
    .gpio_rx =
    {
        .port = IO_PORTB,
        .pin  = PIN_9,
        .mux  = IO_PERIPH_MUX7,
        .pull = IO_PULL_RESERVE,
    },
};

static void ymodem_uart_putc(char c)
{
    struct uart_handle *uart = &uart1;

    uart_putc(uart, c);
}

static int ymodem_uart_getc(char *ch, unsigned int timeout)
{
    int c = 0;
    struct uart_handle *uart = &uart1;

    while (timeout--)
    {
        c = uart_getc(uart);
        if (c >= 0)
        {
            *ch = (char)c;
            return 0;
        }
        ms_delay(1);
    }

    return -1;
}

ymdoem_port_t ymodem_port =
{
    .ymodem_getchar = ymodem_uart_getc,
    .ymodem_putchar = ymodem_uart_putc,
};

int ymodem_register(void)
{
    int ret = 0;

    ret = uart_init(&uart1);

    if (ret != 0)
    {
        s_printf("usrt1 init err.\r\n");
        return 0;
    }

    return ymodem_init(&ymodem_port);
}
