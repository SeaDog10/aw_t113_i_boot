#ifndef __DRV_UART_H__
#define __DRV_UART_H__

#include "drv_gpio.h"

typedef struct {
    unsigned int    base;
    unsigned char   id;
    gpio_mux_t      gpio_tx;
    gpio_mux_t      gpio_rx;
} sunxi_usart_t;

void sunxi_usart_init(sunxi_usart_t *usart);
void sunxi_usart_putc(void *arg, char c);
int sunxi_usart_getc(void *arg);

#endif /* __DRV_UART_H__ */
