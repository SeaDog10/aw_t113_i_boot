#include <drv_twi.h>
#include <board.h>
#include <rtdevice.h>
#include <drv_iomux.h>
#include <drv_clk.h>

#define DBG_TAG "[drv.twi]"
#define BSP_ENBALE_TWI_DEBUG 1

#define DBG_ENABLE
#if (BSP_ENBALE_TWI_DEBUG == 1)
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_WARNING
#endif
#define DBG_COLOR
#include <rtdbg.h>

typedef struct twi_device
{
    rt_uint32_t                 base_addr;
    rt_uint32_t                 irq;
    rt_uint32_t                 id;
    struct iomux_cfg            twi_clk;
    struct iomux_cfg            twi_sda;
    rt_uint32_t                 msg_ptr;
    rt_uint32_t                 error_code;
    rt_bool_t                   lock;
    struct rt_i2c_bus_device    device;
    struct rt_i2c_msg           *msgs;
}*twi_dev_t;

static void twi_enable_irq(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg |= (1 << 7);
    reg &= ~((1 << 3) | (1 << 4) | (1 << 5));
    writel(reg, dev->base_addr + REG_TWI_CNTR);
}

static void twi_disable_irq(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg &= ~((1 << 3) | (1 << 4) | (1 << 5) | (1 << 7));
    writel(reg, dev->base_addr + REG_TWI_CNTR);
}

static void twi_clear_irq(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg |= (1 << 3);
    reg &= ~((1 << 4) | (1 << 5));
    writel(reg, dev->base_addr + REG_TWI_CNTR);
}

static void twi_soft_reset(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    reg = readl(dev->base_addr + REG_TWI_SRST);
    reg |= 1;
    writel(reg, dev->base_addr + REG_TWI_SRST);

    while (readl(dev->base_addr + REG_TWI_SRST));
}

static void twi_put_byte(twi_dev_t dev, rt_uint8_t buffer)
{
    writel(buffer, dev->base_addr + REG_TWI_DATA);
    twi_clear_irq(dev);
}

static void twi_get_byte(twi_dev_t dev, rt_uint8_t *buffer)
{
    *buffer = (rt_uint8_t)(readl(dev->base_addr + REG_TWI_DATA) & 0xFF);
    twi_clear_irq(dev);
}

static void twi_get_last_byte(twi_dev_t dev, rt_uint8_t *buffer)
{
    *buffer = (rt_uint8_t)(readl(dev->base_addr + REG_TWI_DATA) & 0xFF);
}

static void twi_stop(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg |= (1 << 4);
    reg &= ~(1 << 3);
    writel(reg, dev->base_addr + REG_TWI_CNTR);

    twi_clear_irq(dev);
}

static void twi_enable_ack(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg |= (1 << 2);
    reg &= ~(1 << 3);
    writel(reg, dev->base_addr + REG_TWI_CNTR);
}

static void twi_disable_ack(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg &= ~((1 << 2) | (1 << 3));
    writel(reg, dev->base_addr + REG_TWI_CNTR);
}

static int twi_send_clk_9pulse(twi_dev_t dev)
{
    rt_uint32_t reg = 0;
    rt_uint32_t cycle = 0;

    /* enable scl control */
    reg = readl(dev->base_addr + REG_TWI_LCR);
    reg |= (1 << 2);
    writel(reg, dev->base_addr + REG_TWI_LCR);

    while (cycle < 9)
    {
        if ((readl(dev->base_addr + REG_TWI_LCR) & 0x10) == 0x10)
        {
            break;
        }
        /* twi_scl -> low */
        reg = readl(dev->base_addr + REG_TWI_LCR);
        reg &= ~(1 << 3);
        writel(reg, dev->base_addr + REG_TWI_LCR);

        for (int i = 0; i < 100000; i++);

        /* twi_scl -> high */
        reg = readl(dev->base_addr + REG_TWI_LCR);
        reg |= (1 << 3);
        writel(reg, dev->base_addr + REG_TWI_LCR);
        for (int i = 0; i < 100000; i++);

        cycle++;
    }

    reg = readl(dev->base_addr + REG_TWI_LCR);
    reg &= ~(1 << 2);
    writel(reg, dev->base_addr + REG_TWI_LCR);
    if ((readl(dev->base_addr + REG_TWI_LCR) & 0x10) == 0x10)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

static void twi_set_clk(twi_dev_t dev, int clk)
{
    rt_uint32_t reg = 0;

    if (clk == 100000)
    {
        reg = readl(dev->base_addr + REG_TWI_CCR);
        reg &= ~(0x7F);
        reg |= ((11 << 3) | (1 << 0));
        writel(reg, dev->base_addr + REG_TWI_CCR);
    }
    else
    {
        reg = readl(dev->base_addr + REG_TWI_CCR);
        reg &= ~(0x7F);
        reg |= ((0 << 3) | (1 << 0));
        writel(reg, dev->base_addr + REG_TWI_CCR);
    }
}

static void twi_irq_handler(int irqno, void *param)
{
    rt_uint32_t reg = 0;
    rt_uint32_t addr = 0;

    /* enter interrupt */
    rt_interrupt_enter();

    twi_dev_t dev = (twi_dev_t)param;

    twi_disable_irq(dev);

    /* get state */
    reg = readl(dev->base_addr + REG_TWI_STAT);

    switch (reg)
    {
    case 0xF8:
        dev->error_code = 0xF8;
        twi_stop(dev);
        break;

    case 0x08:
    case 0x10:
        addr = (dev->msgs->addr << 1);
        if (dev->msgs->flags == RT_I2C_RD)
        {
            addr |= RT_I2C_RD;
        }
        twi_put_byte(dev, addr);
        break;

    case 0xd8:
    case 0x20:
        dev->error_code = 0x20;
        twi_stop(dev);
        break;

    case 0x18:
    case 0xd0:
    case 0x28:
        if (dev->msg_ptr < dev->msgs->len)
        {
            twi_put_byte(dev, dev->msgs->buf[dev->msg_ptr]);
            dev->msg_ptr++;
            break;
        }

        dev->lock = RT_TRUE;
        dev->msg_ptr = 0;
        twi_stop(dev);
        break;

    case 0x30:
        dev->error_code = 0x30;
        twi_stop(dev);
        break;

    case 0x38:
        dev->error_code = 0x38;
        twi_stop(dev);
        break;

    case 0x40:
        if (dev->msgs->len > 1)
        {
            twi_enable_ack(dev);
        }
        twi_clear_irq(dev);
        break;

    case 0x48:
        dev->error_code = 0x48;
        twi_stop(dev);
        break;

    case 0x50:
        if (dev->msg_ptr < dev->msgs->len)
        {
            /* more than 2 bytes, the last byte need not to send ACK */
            if ((dev->msg_ptr + 2) == dev->msgs->len)
            {
                /* last byte no ACK */
                twi_disable_ack(dev);
            }
            twi_get_byte(dev, &dev->msgs->buf[dev->msg_ptr]);
            dev->msg_ptr++;
            break;
        }
        break;

    case 0x58:
        /* received the last byte  */
        if (dev->msg_ptr == dev->msgs->len - 1)
        {
            twi_get_last_byte(dev, &dev->msgs->buf[dev->msg_ptr]);

            dev->lock = RT_TRUE;
            dev->msg_ptr = 0;
            twi_stop(dev);
        }
        break;

    case 0x00:
        dev->error_code = 0xFF;
        twi_stop(dev);
        break;

    default:
        dev->error_code = reg;
        twi_stop(dev);
        break;
    }

    twi_clear_irq(dev);
    twi_enable_irq(dev);

    /* leave interrupt */
    rt_interrupt_leave();
}

static rt_err_t twi_xfer(twi_dev_t dev)
{
    rt_uint32_t reg = 0;

    dev->lock = RT_FALSE;

    /* start signal, needn't clear int flag */
    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg |= 1 << 5;
    writel(reg, dev->base_addr + REG_TWI_CNTR);

    if (TIMEOUT(10, !dev->lock))
    {
        LOG_E("i2c xfel timeout, error_code:0x%x\n", dev->error_code);
        return (-RT_ERROR);
    }

    return RT_EOK;
}

static rt_size_t t113_twi_master_xfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    twi_dev_t dev = rt_container_of(bus, struct twi_device, device);
    rt_err_t ret = 0;
    rt_uint32_t i = 0;

    /* soft reset */
    twi_soft_reset(dev);
    rt_thread_mdelay(5);

    twi_send_clk_9pulse(dev);

    /* enable irq */
    twi_enable_irq(dev);

    /* set the special function register,default:0. */
    writel(0, dev->base_addr + REG_TWI_EFR);

    for (i = 0; i < num; i++)
    {
        dev->msgs = &msgs[i];
        ret = twi_xfer(dev);
        if (ret != RT_EOK)
        {
            return 0;
        }
    }

    return num;
}

static const struct rt_i2c_bus_device_ops t113_i2c_ops =
{
    .master_xfer        = t113_twi_master_xfer,
    .slave_xfer         = RT_NULL,
    .i2c_bus_control    = RT_NULL
};

#ifdef BSP_USING_TWI0
static struct twi_device twi0 =
{
    .base_addr  = TWI0_BASE_ADDR,
    .irq        = TWI0_IRQ_NUM,
    .id         = 0,
    .lock       = RT_FALSE,
    .msg_ptr    = 0,
    .error_code = 0,
    .device.ops = &t113_i2c_ops,
    .twi_clk    = {.port=IO_PORTB,.pin=PIN_10,.mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE},
    .twi_sda    = {.port=IO_PORTB,.pin=PIN_11,.mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE},
};
#endif /* BSP_USING_TWI0 */

static void _twi_init(twi_dev_t dev)
{
    rt_uint32_t reg = 0;
    rt_uint32_t addr = 0;

    iomux_set_sel(&dev->twi_clk);
    iomux_set_sel(&dev->twi_sda);

    addr = CCU_BASE_ADDR + REG_CCU_TWI_BGR;

    reg = readl(addr);
    reg |= (1 << (dev->id));
    writel(reg, addr);

    reg = readl(addr);
    reg |= (1 << (dev->id + 16));
    writel(reg, addr);

    /* config speed */
    twi_set_clk(dev, 100000);

    /* soft reset */
    reg = readl(dev->base_addr + REG_TWI_SRST);
    reg |= 0x1;
    writel(reg, dev->base_addr + REG_TWI_SRST);

    /* enable irq and i2c */
    reg = readl(dev->base_addr + REG_TWI_CNTR);
    reg |= ((1 << 6) | (1 << 7));
    writel(reg, dev->base_addr + REG_TWI_CNTR);
}

int rt_hw_twi_init(void)
{
    int ret = 0;

#ifdef BSP_USING_TWI0
    _twi_init(&twi0);

    rt_hw_interrupt_install(twi0.irq, twi_irq_handler, &twi0, "twi0");
    rt_hw_interrupt_umask(twi0.irq);

    ret = rt_i2c_bus_device_register(&twi0.device, "twi0");
    if(ret != RT_EOK)
    {
        rt_kprintf("twi0 register fail\n");
        return (-RT_ERROR);
    }
#endif /* BSP_USING_TWI0 */

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_twi_init);
