#include "interrupt.h"
#include "cp15.h"
#include "drv_uart.h"
#include "drv_iomux.h"
#include "memheap.h"
#include "shell.h"

struct uart_handle uart0 = {
    .base     = UART0_BASE_ADDR,
    .irq      = UART0_IRQ,
    .id       = UART0,
    .cfg      =
    {
       .baud_rate = BAUD_RATE_115200,
       .data_bits = UART_DATA_BITS_8,
       .stop_bits = UART_STOP_BITS_1,
       .parity    = UART_PARITY_NONE,
    },
    .gpio_tx =
    {
        .port = IO_PORTG,
        .pin  = PIN_17,
        .mux  = IO_PERIPH_MUX7,
        .pull = IO_PULL_RESERVE,
    },
    .gpio_rx =
    {
        .port = IO_PORTG,
        .pin  = PIN_18,
        .mux  = IO_PERIPH_MUX7,
        .pull = IO_PULL_RESERVE,
    },
};

static void shell_uart_putc(void *arg, char c)
{
    struct uart_handle *uart = (struct uart_handle *)arg;

    uart_putc(uart, c);
}

static char shell_uart_getc(void *arg)
{
    struct uart_handle *uart = (struct uart_handle *)arg;

    return uart_getc(uart);
}

static int memfree(int argc, char **argv);
static struct shell_command free_cmd =
{
    .name = "memfree",
    .desc = "show memheap info",
    .func = memfree,
    .next = SHELL_NULL,
};

static int memfree(int argc, char **argv)
{
    int ret = 0;
    unsigned int total   = 0;
    unsigned int used    = 0;
    unsigned int maxused = 0;

    s_printf("__memheap_start : 0x%08x, __memheap_end : 0x%08x\r\n", (unsigned int)HEAP_BEGIN, (unsigned int)HEAP_END);
    ret = rt_memheap_init(&system_heap, HEAP_BEGIN, HEAP_SIZE);
    if (ret != 0)
    {
        s_printf("memheap init failed\r\n");
    }
    else
    {
        rt_memheap_info(&system_heap, &total, &used, &maxused);
        s_printf("memheap init success, total: 0x%08x, used: 0x%08x, maxused: 0x%08x\r\n", total, used, maxused);
    }

    return 0;
}

int main(void)
{
    interrupt_disable();
    interrupt_init();
    interrupt_enable();

    uart_init(&uart0);

    shell_init(shell_uart_putc, &uart0, shell_uart_getc, &uart0);

    shell_register_command(&free_cmd);

    s_printf("hello world\r\n");

    while(1)
    {
        shell_servise();
    }
}


