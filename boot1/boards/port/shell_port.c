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
 version:V1.0.0\r\n";

static void shell_uart_putc(char c)
{
    uart_putc(&uart0, c);
}

static char shell_uart_getc(void)
{
    int ret = 0;

    ret = uart_getc(&uart0);

    if (ret < 0)
    {
        return -1;
    }

    return (char)ret;
}

static shell_port_t shell_port =
{
    .shell_putchar = shell_uart_putc,
    .shell_getchar = shell_uart_getc,
};

int shell_register(void)
{
    int ret = 0;

    ret = uart_init(&uart0);
    if (ret != 0)
    {
        return ret;
    }

    return shell_init(&shell_port, start_logo);
}
