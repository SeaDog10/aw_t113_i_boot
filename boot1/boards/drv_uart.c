#include "drv_uart.h"
#include "interrupt.h"

#define UART_SORCE_CLK    24000000

static void (*uart_recv_callback)(void *param) = U_NULL;

static int _uart_init(struct uart_handle *uart)
{
    unsigned int time_out = 1000;
    unsigned int addr;
    unsigned int val;

    if (uart == U_NULL)
    {
        return -1;
    }

    /* Config usart TXD and RXD pins */
    iomux_set_sel(&(uart->gpio_tx));
    iomux_set_sel(&(uart->gpio_rx));

    /* Open the clock gate for uart */
    addr = 0x02001000 + 0x90c;
    val  = read32(addr);
    val |= 1 << uart->id;
    write32(addr, val);

    /* Deassert USART reset */
    addr = 0x02001000 + 0x90c;
    val  = read32(addr);
    val |= 1 << (16 + uart->id);
    write32(addr, val);

    addr = uart->base;

    /* hold tx so that uart will update lcr and baud in the gap of tx */
    val = read32(addr + REG_UART_HALT);
    val &= ~(0x3 << 0);
    val |= (0x3 << 0);
    write32(addr + REG_UART_HALT, val);

    val = read32(addr + REG_UART_LCR);
    val &= ~(2 << 0);
    switch (uart->cfg.data_bits)
    {
    case UART_DATA_BITS_5:
        break;
    case UART_DATA_BITS_6:
        val |= 0x1;
        break;
    case UART_DATA_BITS_7:
        val |= 0x2;
        break;
    case UART_DATA_BITS_8:
        val |= 0x3;
        break;
    default:
        return -1;
        break;
    }

    val &= ~(1 << 2);
    switch (uart->cfg.stop_bits)
    {
    case UART_STOP_BITS_1:
        break;
    case UART_STOP_BITS_1_5:
        val |= 0x1 << 2;
        break;
    default:
        return -1;
        break;
    }

    val &= ~(0x7 << 3);
    switch (uart->cfg.parity)
    {
    case UART_PARITY_NONE:
        break;
    case UART_PARITY_ODD:
        val |= 0x1 << 3;
        break;
    case UART_PARITY_EVEN:
        val |= 0x1 << 3;
        val |= 0x1 << 4;
        break;
    default:
        return -1;
        break;
    }
    write32(addr + REG_UART_LCR, val);

    /* set DLAB to access DLL and DLH registers */
    val = read32(addr + REG_UART_LCR);
    val |= 0x1 << 7;
    write32(addr + REG_UART_LCR, val);
    /* set baud div */
    write32(addr + REG_UART_DLL, (unsigned int)(UART_SORCE_CLK / (16 * uart->cfg.baud_rate)) & 0xff);
    write32(addr + REG_UART_DLH, ((unsigned int)(UART_SORCE_CLK / (16 * uart->cfg.baud_rate)) >> 8) & 0xff);
    /* clear DLAB */
    val = read32(addr + REG_UART_LCR);
    val &= ~(0x1 << 7);
    write32(addr + REG_UART_LCR, val);

    /* enable fifo */
    write32(addr + REG_UART_FCR, 0xf7);

    /* set mode */
    write32(addr + REG_UART_MCR, 0x0);

    val = read32(addr + REG_UART_HALT);
    val &= ~(0x1 << 2);
    val &= ~(0x1 << 0);
    val |= (0x1 << 2);
    write32(addr + REG_UART_HALT, val);

    /* wait config update */
    while ((read32(addr + REG_UART_HALT) & (0x1 << 2)) && time_out--);

    if (time_out == 0)
    {
        return -1;
    }

    return 0;
}

static void uart_irq_handler(int irqno, void *param)
{
    unsigned int i = 0;
    unsigned int rx_len = 0;
    struct uart_handle *uart = (struct uart_handle *)param;

    /* get fifo level */
    rx_len = read32(uart->base + REG_UART_RFL);
    if (rx_len > 0)
    {
        /* read char */
        for (i = 0; i < rx_len; i++)
        {
            uart->fifo.buffer[uart->fifo.put_index] = (char)read32(uart->base + REG_UART_RBR)  & 0xFF;
            uart->fifo.put_index += 1;
            if (uart->fifo.put_index >= UART_SOFT_FIFO_SIZE)
            {
                uart->fifo.put_index = 0;
            }

            if (uart->fifo.put_index == uart->fifo.get_index)
            {
                uart->fifo.get_index += 1;
                uart->fifo.is_full    = 1;
                if (uart->fifo.get_index >= UART_SOFT_FIFO_SIZE)
                {
                    uart->fifo.get_index = 0;
                }
            }
        }

        if (uart_recv_callback)
        {
            uart_recv_callback(param);
        }
    }
}

int uart_init(struct uart_handle *uart)
{
    int ret = 0;
    int i = 0;
    unsigned int val = 0;

    ret = _uart_init(uart);
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; i < UART_SOFT_FIFO_SIZE; i++)
    {
        uart->fifo.buffer[i] = 0;
    }
    uart->fifo.put_index = 0;
    uart->fifo.get_index = 0;
    uart->fifo.is_full = 0;

    interrupt_install(uart->irq, uart_irq_handler, uart);
    interrupt_umask(uart->irq);

    /* enable irq */
    val = read32(uart->base + REG_UART_IER);
    val |= ((0x1 << 0) | (0x1 << 2));
    write32(uart->base + REG_UART_IER, val);

    return 0;
}

int uart_deinit(struct uart_handle *uart)
{
    unsigned int val = 0;

    if (uart == U_NULL)
    {
        return -1;
    }

    /* disable irq */
    val = read32(uart->base + REG_UART_IER);
    val &= ~((0x1 << 0) | (0x1 << 2));
    write32(uart->base + REG_UART_IER, val);
    interrupt_mask(uart->irq);

    return 0;
}

int uart_putc(struct uart_handle *uart, char c)
{
    unsigned int time_out = 1000;
    int ret = 0;

    if (uart == U_NULL)
    {
        return -1;
    }

    while ((!(read32(uart->base + REG_UART_USR) & (0x1 << 2))) && (time_out--));
    if (time_out == 0)
    {
        ret = -1;
    }
    else
    {
        write32(uart->base + REG_UART_THR, c);
        ret = 0;
    }

    return ret;
}

char uart_getc(struct uart_handle *uart)
{
    char ch = -1;

    if (uart == U_NULL)
    {
        return -1;
    }

    if ((uart->fifo.get_index == uart->fifo.put_index) && uart->fifo.is_full == 0)
    {
        return -1;
    }

    ch = uart->fifo.buffer[uart->fifo.get_index];
    uart->fifo.get_index += 1;

    if (uart->fifo.get_index >= UART_SOFT_FIFO_SIZE)
    {
        uart->fifo.get_index = 0;
    }

    if (uart->fifo.is_full == 1)
    {
        uart->fifo.is_full = 0;
    }

    return ch;
}

int uart_bind_recv_callback(void (*callback)(void *param))
{
    uart_recv_callback = callback;

    return 0;
}
