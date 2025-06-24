#include "ymodem.h"
#include "drv_uart.h"
#include "board.h"
#include "shell.h"

extern struct uart_handle uart0;
static struct uart_handle uart1 =
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

static void ymodem_uart_putc(void *arg, char c)
{
    struct uart_handle *uart = (struct uart_handle *)arg;

    uart_putc(uart, c);
}

static int ymodem_uart_getc(void *arg, char *ch, unsigned int timeout)
{
    int c = 0;
    struct uart_handle *uart = (struct uart_handle *)arg;

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

int ymodem_register(void)
{
    int ret = 0;

    ret = uart_init(&uart1);

    if (ret != 0)
    {
        s_printf("usrt1 init err.\r\n");
        return 0;
    }

    return ymodem_init(ymodem_uart_putc, &uart1, ymodem_uart_getc, &uart1);
}
