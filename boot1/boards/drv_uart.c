#include "drv_uart.h"
#include "board.h"
#include "interrupt.h"

#define T113_CCU_BASE       (0x02001000)
#define CCU_USART_BGR_REG   (0x90c)

void sunxi_usart_init(sunxi_usart_t *usart)
{
    unsigned int addr;
    unsigned int val;

    /* Config usart TXD and RXD pins */
    sunxi_gpio_init(usart->gpio_tx.pin, usart->gpio_tx.mux);
    sunxi_gpio_init(usart->gpio_rx.pin, usart->gpio_rx.mux);

    /* Open the clock gate for usart */
    addr = T113_CCU_BASE + CCU_USART_BGR_REG;
    val     = read32(addr);
    val |= 1 << usart->id;
    write32(addr, val);

    /* Deassert USART reset */
    addr = T113_CCU_BASE + CCU_USART_BGR_REG;
    val     = read32(addr);
    val |= 1 << (16 + usart->id);
    write32(addr, val);

    /* Config USART to 115200-8-1-0 */
    addr = usart->base;
    write32(addr + 0x04, 0x0);
    write32(addr + 0x08, 0xf7);
    write32(addr + 0x10, 0x0);
    val = read32(addr + 0x0c);
    val |= (1 << 7);
    write32(addr + 0x0c, val);
    write32(addr + 0x00, 0xd & 0xff);
    write32(addr + 0x04, (0xd >> 8) & 0xff);
    val = read32(addr + 0x0c);
    val &= ~(1 << 7);
    write32(addr + 0x0c, val);
    val = read32(addr + 0x0c);
    val &= ~0x1f;
    val |= (0x3 << 0) | (0 << 2) | (0x0 << 3);
    write32(addr + 0x0c, val);
}

void sunxi_usart_putc(void *arg, char c)
{
    sunxi_usart_t *usart = (sunxi_usart_t *)arg;

    while ((read32(usart->base + 0x7c) & (0x1 << 1)) == 0);
    write32(usart->base + 0x00, c);
}

int sunxi_usart_getc(void *arg)
{
    sunxi_usart_t *usart = (sunxi_usart_t *)arg;

    if (read32(usart->base + 0x7c) & (0x1 << 3))
    {
        return read32(usart->base + 0x00) & 0xff;
    }

    return -1;
}

static void uart_irq_handler(int irqno, void *param)
{
    sunxi_usart_t *usart = (sunxi_usart_t *)param;
    unsigned int val = 0;

    val = read32(usart->base + 0x08) & 0x0f;

    if ((val & 0x04))
    {
        sunxi_usart_putc(usart, (char)sunxi_usart_getc(usart));
    }
}

void uart_hw_init(sunxi_usart_t *usart)
{
    sunxi_usart_init(usart);

    interrupt_install(34, uart_irq_handler, usart, "uart");
    interrupt_umask(34);
    write32(usart->base + 0x04, 0x1);
}
