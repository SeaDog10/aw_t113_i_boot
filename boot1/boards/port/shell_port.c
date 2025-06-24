#include "shell_port.h"
#include "drv_uart.h"
#include "drv_iomux.h"

extern struct uart_handle uart0;

static const char *start_logo =
"                                                     \r\n\
  _    _  _____        ____   ____   ____ _______     \r\n\
 | |  | |/ ____|      |  _ \\ / __ \\ / __ \\__   __| \r\n\
 | |__| | |  __ ______| |_) | |  | | |  | | | |       \r\n\
 |  __  | | |_ |______|  _ <| |  | | |  | | | |       \r\n\
 | |  | | |__| |      | |_) | |__| | |__| | | |       \r\n\
 |_|  |_|\\_____|      |____/ \\____/ \\____/  |_|    \r\n\
 version:V0.0.1\r\n";

static void shell_uart_putc(void *arg, char c)
{
    struct uart_handle *uart = (struct uart_handle *)arg;

    uart_putc(uart, c);
}

static char shell_uart_getc(void *arg)
{
    int ret = 0;

    struct uart_handle *uart = (struct uart_handle *)arg;

    ret = uart_getc(uart);

    if (ret < 0)
    {
        return -1;
    }

    return (char)ret;
}

int shell_register(void)
{
    int ret = 0;

    ret = uart_init(&uart0);
    if (ret != 0)
    {
        return ret;
    }

    return shell_init(shell_uart_putc, &uart0, shell_uart_getc, &uart0, start_logo);
}
