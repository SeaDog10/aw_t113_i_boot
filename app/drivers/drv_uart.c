#include <rtdevice.h>
#include <rtdef.h>
#include <board.h>
#include <drv_uart.h>
#include <drv_iomux.h>
#include <drv_clk.h>

typedef struct uart_device
{
    struct rt_serial_device paret;
    rt_uint32_t             base;
    rt_uint32_t             irq;
    rt_uint32_t             id;
    struct iomux_cfg        gpio_tx;
    struct iomux_cfg        gpio_rx;
    rt_uint32_t             rx_len;
    rt_uint32_t             index_num;
    char                    rx_buf[256];
}*uart_device_t;

static void _uart_init(struct uart_device *dev)
{
    rt_uint32_t addr;
    rt_uint32_t val;

    /* Config usart TXD and RXD pins */
    iomux_set_sel(&(dev->gpio_tx));
    iomux_set_sel(&(dev->gpio_rx));

    /* Open the clock gate for uart */
    addr = 0x02001000 + 0x90c;
    val  = readl(addr);
    val |= 1 << dev->id;
    writel(val, addr);

    /* Deassert USART reset */
    addr = 0x02001000 + 0x90c;
    val  = readl(addr);
    val |= 1 << (16 + dev->id);
    writel(val, addr);
}

/*
 * UART interface
 */
static rt_err_t uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    uart_device_t dev = (uart_device_t)serial;
    rt_uint32_t time_out = 1000;
    rt_uint32_t addr;
    rt_uint32_t val;
    rt_uint32_t sclk = 0;

    if (serial == RT_NULL || cfg == RT_NULL)
    {
        return -RT_EINVAL;
    }

    addr = dev->base;
    /* hold tx so that uart will update lcr and baud in the gap of tx */
    val = readl(addr + REG_UART_HALT);
    val &= ~(0x3 << 0);
    val |= (0x3 << 0);
    writel(val, addr + REG_UART_HALT);

    val = readl(addr + REG_UART_LCR);
    val &= ~(2 << 0);

    switch (cfg->data_bits)
    {
    case DATA_BITS_5:
        break;
    case DATA_BITS_6:
        val |= 0x1;
        break;
    case DATA_BITS_7:
        val |= 0x2;
        break;
    case DATA_BITS_8:
        val |= 0x3;
        break;
    default:
        break;
    }

    val &= ~(1 << 2);
    switch (cfg->stop_bits)
    {
    case STOP_BITS_1:
        break;
    default:
        break;
    }

    val &= ~(0x7 << 3);
    switch (cfg->parity)
    {
    case PARITY_NONE:
        break;
    case PARITY_ODD:
        val |= 0x1 << 3;
        break;
    case PARITY_EVEN:
        val |= 0x1 << 3;
        val |= 0x1 << 4;
        break;
    default:
        break;
    }
    writel(val, addr + REG_UART_LCR);

    /* set DLAB to access DLL and DLH registers */
    val = readl(addr + REG_UART_LCR);
    val |= 0x1 << 7;
    writel(val, addr + REG_UART_LCR);
    /* set baud div */
    sclk = drv_clk_get_apb1_clk();
    writel((unsigned int)(sclk / (16 * cfg->baud_rate)) & 0xff, addr + REG_UART_DLL);
    writel(((unsigned int)(sclk / (16 * cfg->baud_rate)) >> 8) & 0xff, addr + REG_UART_DLH);
    /* clear DLAB */
    val = readl(addr + REG_UART_LCR);
    val &= ~(0x1 << 7);
    writel(val, addr + REG_UART_LCR);

    /* enable fifo */
    writel(0xf7, addr + REG_UART_FCR);

    /* set mode */
    writel(0x00, addr + REG_UART_MCR);

    val = readl(addr + REG_UART_HALT);
    val &= ~(0x1 << 2);
    val &= ~(0x1 << 0);
    val |= (0x1 << 2);
    writel(val, addr + REG_UART_HALT);

    /* wait config update */
    while ((readl(addr + REG_UART_HALT) & (0x1 << 2)) && time_out--);

    if (time_out == 0)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    uart_device_t dev = (uart_device_t)serial;
    rt_uint32_t val = 0;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable irq */
        val = readl(dev->base + REG_UART_IER);
        val &= ~((0x1 << 0) | (0x1 << 2));
        writel(val, dev->base + REG_UART_IER);
        rt_hw_interrupt_mask(dev->irq);
        break;
    case RT_DEVICE_CTRL_SET_INT:
        /* enable irq */
        val = readl(dev->base + REG_UART_IER);
        val |= ((0x1 << 0) | (0x1 << 2));
        writel(val, dev->base + REG_UART_IER);
        rt_hw_interrupt_umask(dev->irq);
        break;
    default:
        break;
    }

    return RT_EOK;
}


static int uart_putc(struct rt_serial_device *serial, char c)
{
    uart_device_t dev = (uart_device_t)serial;
    int ret = 0;
    rt_uint32_t time_out = 1000;

    while((!(readl(dev->base + REG_UART_USR) & (0x1 << 2))) && (time_out--));
    if (time_out == 0)
    {
        ret = 0;
    }
    else
    {
        writel(c, dev->base + REG_UART_THR);
        ret = 1;
    }

    return ret;
}

static int uart_getc(struct rt_serial_device *serial)
{
    uart_device_t dev = (uart_device_t)serial;

    if (dev->index_num >= dev->rx_len)
    {
        dev->index_num = 0;
        dev->rx_len = 0;
        return (-RT_ERROR);
    }

    return (int)dev->rx_buf[dev->index_num++];
}

static void uart_irq_handler(int irqno, void *param)
{
    uart_device_t dev = (uart_device_t)param;
    rt_uint32_t i = 0;

    /* enter interrupt */
    rt_interrupt_enter();

    /* get fifo level */
    dev->rx_len = readl(dev->base + REG_UART_RFL);
    if (dev->rx_len > 0)
    {
        /* read char */
        for (i = 0; i < dev->rx_len; i++)
        {
            dev->rx_buf[i++] = (char)readl(dev->base + REG_UART_RBR)  & 0xFF;;
        }
        rt_hw_serial_isr(&dev->paret, RT_SERIAL_EVENT_RX_IND);
    }

    /* leave interrupt */
    rt_interrupt_leave();
}

static const struct rt_uart_ops _uart_ops =
{
    .configure = uart_configure,
    .control = uart_control,
    .putc = uart_putc,
    .getc = uart_getc,
};

/*
 * UART Initiation
 */
#ifdef RT_USING_UART0
static struct uart_device uart0 =
{
    .base   = UART0_BASE_ADDR,
    .irq    = UART0_IRQ,
    .id     = 0,
    .rx_len = 0,
    .index_num = 0,
    .gpio_rx = {.port = IO_PORTG, .pin = PIN_18, .mux = IO_PERIPH_MUX7, .pull = IO_PULL_RESERVE},
    .gpio_tx = {.port = IO_PORTG, .pin = PIN_17, .mux = IO_PERIPH_MUX7, .pull = IO_PULL_RESERVE},
    .paret.ops = &_uart_ops,
};
#endif

int rt_hw_uart_init(void)
{
    rt_err_t ret = RT_EOK;
    struct serial_configure default_config = RT_SERIAL_CONFIG_DEFAULT;

#ifdef RT_USING_UART0
    _uart_init(&uart0);

    rt_memset(&uart0.rx_buf[0], 0, sizeof(uart0.rx_buf));
    rt_hw_interrupt_install(uart0.irq, uart_irq_handler, &uart0, "uart0");
    rt_hw_interrupt_umask(uart0.irq);

    uart0.paret.config = default_config;
    ret = rt_hw_serial_register(&uart0.paret, "uart0", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, &uart0);
#endif /* RT_USING_UART0 */

    return ret;
}
