#include "main.h"
#include "interrupt.h"
#include "cp15.h"
#include "drv_uart.h"
#include "drv_gpio.h"

sunxi_usart_t usart0_dbg = {
    .base     = 0x02500000,
    .id         = 0,
    .gpio_tx = {GPIO_PIN(PORTG, 17), GPIO_PERIPH_MUX7},
    .gpio_rx = {GPIO_PIN(PORTG, 18), GPIO_PERIPH_MUX7},
};

int startup(void)
{
    interrupt_disable();

    interrupt_init();

    interrupt_enable();

    /* heap init */

    /* uart_init */

    /* show logo */
}

int main(void)
{
    int ch = -1;

    startup();

    // sunxi_usart_init(&usart0_dbg);
    uart_hw_init(&usart0_dbg);

    sunxi_usart_putc(&usart0_dbg, 'h');
    sunxi_usart_putc(&usart0_dbg, 'e');
    sunxi_usart_putc(&usart0_dbg, 'l');
    sunxi_usart_putc(&usart0_dbg, 'l');
    sunxi_usart_putc(&usart0_dbg, 'o');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');

    sunxi_usart_putc(&usart0_dbg, '1');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');

    while(1)
    {
    }
}