#include <drv_emac.h>
#include <drv_iomux.h>
#include <drv_clk.h>
#include <board.h>
#include "netif/ethernetif.h"

#define EMAC_RX_FRAME_MAX    30
#define EMAC_DMA_TX_DESC_NUM 128
#define EMAC_DMA_RX_DESC_NUM 128
#define RX_BUF_SIZE          1536
#define TX_BUF_SIZE          1536

typedef struct emac_device
{
    rt_uint32_t base_addr;
    rt_uint32_t irq_num;
    volatile rt_bool_t  tx_completed;
    volatile rt_bool_t  rx_completed;
    struct eth_device device;
} *emac_dev_t;

typedef struct dma_desc
{
    volatile rt_uint32_t desc0; /* 1st: Status */
    volatile rt_uint32_t desc1; /* 2nd: Buffer Size */
    volatile rt_uint32_t desc2; /* 3rd: Buffer Addr */
    volatile rt_uint32_t desc3; /* 4th: Next Desc */
    volatile rt_uint32_t reserve[4];
} dma_desc_t;

struct tx_ring_desc_mgr
{
    dma_desc_t *tx_desc_pool;
    rt_uint16_t current_proc_index;
};

struct rx_ring_desc_mgr
{
    dma_desc_t *rx_desc_pool;
    rt_uint16_t current_proc_index;
};

static rt_uint32_t rx_count = 0;
static struct rx_ring_desc_mgr rx_ring = {0};
static struct tx_ring_desc_mgr tx_ring = {0};
static rt_uint8_t tx_buffer[EMAC_DMA_TX_DESC_NUM][TX_BUF_SIZE] __attribute__((aligned(32))) = {0};
static rt_uint8_t rx_buffer[EMAC_DMA_RX_DESC_NUM][RX_BUF_SIZE] __attribute__((aligned(32))) = {0};
static rt_uint8_t mac_addr[6] = {0x5a, 0x02, 0x03, 0x04, 0x05, 0x06};

static void emac_irq_handle(int vector, void *param)
{
    rt_uint32_t status = 0;
    struct emac_device *dev = (struct emac_device *)param;

    rt_interrupt_enter();

    rt_hw_interrupt_mask(dev->irq_num);
    writel(0x00, dev->base_addr + REG_EMAC_INT_EN);
    /* clear interrupt status */
    status = readl(dev->base_addr + REG_EMAC_INT_STA);
    writel(status, dev->base_addr + REG_EMAC_INT_STA);

    rx_count = 0;

    if (status & (1 << 13))
    {
        eth_device_ready(&dev->device);
    }

    rt_interrupt_leave();
}

static rt_err_t emac_dma_tx_desc_init(void)
{
    int i = 0;
    rt_err_t ret = RT_EOK;

    tx_ring.current_proc_index = 0;

    for (i = 0; i < EMAC_DMA_TX_DESC_NUM; i++)
    {
        tx_ring.tx_desc_pool[i].desc0 = 0;
        tx_ring.tx_desc_pool[i].desc1 = 0;
        tx_ring.tx_desc_pool[i].desc2 = (rt_uint32_t)&tx_buffer[i][0];
        if ((i + 1) < EMAC_DMA_TX_DESC_NUM)
        {
            tx_ring.tx_desc_pool[i].desc3 = (rt_uint32_t)&tx_ring.tx_desc_pool[i + 1];
        }
        else
        {
            tx_ring.tx_desc_pool[i].desc3 = (rt_uint32_t)&(tx_ring.tx_desc_pool[0]);
        }
    }

    return ret;
}

static rt_err_t emac_dma_rx_desc_init(void)
{
    int i = 0;
    rt_err_t ret = RT_EOK;

    rx_ring.current_proc_index = 0;

    for (i = 0; i < EMAC_DMA_RX_DESC_NUM; i++)
    {
        rx_ring.rx_desc_pool[i].desc0  = 0x80000000;
        rx_ring.rx_desc_pool[i].desc1  = 0x80000000;
        rx_ring.rx_desc_pool[i].desc1 |= (RX_BUF_SIZE & 0x7ff);
        rx_ring.rx_desc_pool[i].desc2 = (rt_uint32_t)&rx_buffer[i][0];
        if ((i + 1) < EMAC_DMA_RX_DESC_NUM)
        {
            rx_ring.rx_desc_pool[i].desc3 = (rt_uint32_t)&rx_ring.rx_desc_pool[i + 1];
        }
        else
        {
            rx_ring.rx_desc_pool[i].desc3 = (rt_uint32_t)&(rx_ring.rx_desc_pool[0]);
        }
    }

    return ret;
}

static rt_uint16_t emac_phy_read(emac_dev_t dev, rt_uint8_t phy_addr, rt_uint8_t reg)
{
    rt_uint32_t value = 0;

    value |= (0x06 << 20);
    value |= (((phy_addr << 12) & (0x0001F000)) | ((reg << 4) & (0x000001f0)) | (0x01));
    writel(value, dev->base_addr + REG_EMAC_MII_CMD);

    while(readl(dev->base_addr + REG_EMAC_MII_CMD) & 0x01);

    return (rt_uint16_t)(readl(dev->base_addr + REG_EMAC_MII_DATA) & 0xffff);
}

static void emac_phy_write(emac_dev_t dev, rt_uint8_t phy_addr, rt_uint8_t reg, rt_uint16_t data)
{
    rt_uint32_t value = 0;

    value |= (0x06 << 20);
    value |= (((phy_addr << 12) & (0x0001F000)) | ((reg << 4) & (0x000001f0)) | (0x03));
    writel(data, dev->base_addr + REG_EMAC_MII_DATA);
    writel(value, dev->base_addr + REG_EMAC_MII_CMD);

    while(readl(dev->base_addr + REG_EMAC_MII_CMD) & 0x01);
}

static void emac_write_hwaddr(emac_dev_t dev, rt_uint8_t *addr)
{
    rt_uint32_t mac = 0;

    mac  = (addr[5] << 8) | addr[4];
    writel(mac, dev->base_addr + REG_EMAC_ADDR_HIGH0);

    mac  = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
    writel(mac, dev->base_addr + REG_EMAC_ADDR_LOW0);
}

static void dma_tx_enable(emac_dev_t dev, rt_bool_t en)
{
    rt_uint32_t reg_val = 0;

    reg_val = readl(dev->base_addr + REG_EMAC_TX_CTL1);

    if (en == RT_TRUE)
    {
        reg_val |= 0x40000000;
    }
    else
    {
        reg_val &= ~0x40000000;
    }

    writel(reg_val, dev->base_addr + REG_EMAC_TX_CTL1);
}

static void emac_fill_tx_desc(emac_dev_t dev)
{
    dma_desc_t *tx_p = &tx_ring.tx_desc_pool[0];

    dma_tx_enable(dev, RT_FALSE);

    writel((rt_uint32_t)tx_p, dev->base_addr + REG_EMAC_TX_DMA_DESC_LIST);

    dma_tx_enable(dev, RT_TRUE);
}


static void dma_rx_enable(emac_dev_t dev, rt_bool_t en)
{
    rt_uint32_t reg_val = 0;

    reg_val = readl(dev->base_addr + REG_EMAC_RX_CTL1);

    if (en == RT_TRUE)
    {
        reg_val |= 0x40000000;
    }
    else
    {
        reg_val &= ~0x40000000;
    }

    writel(reg_val, dev->base_addr + REG_EMAC_RX_CTL1);
}

static void emac_fill_rx_desc(emac_dev_t dev)
{
    dma_desc_t *rx_p = &rx_ring.rx_desc_pool[0];

    dma_rx_enable(dev, RT_FALSE);

    writel((rt_uint32_t)rx_p, dev->base_addr + REG_EMAC_RX_DMA_DESC_LIST);

    dma_rx_enable(dev, RT_TRUE);
}

static rt_err_t t113_emac_tx(rt_device_t net_dev, struct pbuf *p)
{
    struct emac_device *dev = rt_container_of(net_dev, struct emac_device, device);
    rt_uint32_t reg_val = 0;
    dma_desc_t *tx_p = RT_NULL;
    rt_uint8_t *tx_buf = RT_NULL;
    rt_uint32_t offset = 0;
    rt_uint32_t time_out = 100;
    struct pbuf *q = RT_NULL;

    if (p->tot_len > TX_BUF_SIZE)
    {
        rt_kprintf("eth tx len > TX_BUF_SIZE, %d\n", p->tot_len);
        return -RT_ERROR;
    }

    tx_p = &tx_ring.tx_desc_pool[tx_ring.current_proc_index];
    reg_val = readl(dev->base_addr + REG_EMAC_TX_DMA_STA);
    while((tx_p->desc0 & (1 << 31)) || (reg_val != 0x00 && reg_val != 0x06))
    {
        if (time_out > 0)
        {
            rt_hw_us_delay(1);
            reg_val = readl(dev->base_addr + REG_EMAC_TX_DMA_STA);
            time_out--;
        }
        else
        {
            rt_kprintf("tx desc %d is busy\n", tx_ring.current_proc_index);
            return -RT_ERROR;
        }
    }

    tx_buf = &tx_buffer[tx_ring.current_proc_index][0];

    for (q = p; q != NULL; q = q->next)
    {
        rt_memcpy((rt_uint8_t *)(tx_buf + offset), (rt_uint8_t *)((rt_uint8_t *)q->payload), q->len);
        offset += q->len;
    }

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, (void *)&tx_buffer[tx_ring.current_proc_index][0], offset);

    tx_p->desc0 = 0x80000000; /* Set Own */
    tx_p->desc1 = 0x60000000; /* Set first pkg and last pkg */
    tx_p->desc1 |= (offset & 0x7FF);
    tx_ring.current_proc_index = (tx_ring.current_proc_index + 1) % EMAC_DMA_TX_DESC_NUM;

    /* Enable transmit and Poll transmit */
    reg_val = readl(dev->base_addr + REG_EMAC_TX_CTL1);
    reg_val |= 0xC0000000;
    writel(reg_val, dev->base_addr + REG_EMAC_TX_CTL1);

    return RT_EOK;
}

static struct pbuf *t113_emac_rx(rt_device_t net_dev)
{
    struct emac_device *dev = rt_container_of(net_dev, struct emac_device, device);
    struct pbuf *rx_puf = RT_NULL;
    rt_uint32_t len = 0;
    rt_uint32_t over_flag = 0;
    dma_desc_t *rx_p = RT_NULL;

    rx_p = &rx_ring.rx_desc_pool[rx_ring.current_proc_index];
    if (rx_p->desc0 & (1 << 31))
    {
        over_flag = 1;
        goto exit;
    }

    len = ((rx_p->desc0 >> 16) & 0x3fff);
    rx_puf = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (rx_puf != RT_NULL)
    {
        rx_p->desc0 = 0x80000000;
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, (void *)&rx_buffer[rx_ring.current_proc_index][0], len);
        rt_memcpy(rx_puf->payload, &rx_buffer[rx_ring.current_proc_index][0], len);
        rx_ring.current_proc_index = (rx_ring.current_proc_index + 1) % EMAC_DMA_RX_DESC_NUM;
    }

    rx_count++;

exit:
    if ((rx_count > EMAC_RX_FRAME_MAX) || (over_flag == 1))
    {
        rx_count = 0;
        writel((0x1 << 13), dev->base_addr + REG_EMAC_INT_EN);
        rt_hw_interrupt_umask(dev->irq_num);
    }

    return rx_puf;
}

static rt_err_t t113_emac_control(rt_device_t net_dev, int cmd, void *args)
{
    struct emac_device *dev = rt_container_of(net_dev, struct emac_device, device);

    switch (cmd)
    {
    case NIOCTL_GADDR:
        if (args)
        {
            emac_write_hwaddr(dev, (rt_uint8_t *)args);
        }
        else
        {
            return (-RT_ERROR);
        }
        break;

    default:
        break;
    }

    return RT_EOK;
}

static void emac_phy_init(emac_dev_t dev)
{
    int i = 0;
    rt_uint32_t reg_val = 0;
    rt_uint16_t phy_val = 0;
    rt_uint8_t phy_addr = 0;

    /* reset phy */
    phy_val = emac_phy_read(dev, phy_addr, 0x00);
    emac_phy_write(dev, phy_addr, 0x00, phy_val | 0x8000);
    while (emac_phy_read(dev, phy_addr, 0x00) & 0x8000);

    phy_val = emac_phy_read(dev, phy_addr, 0x00);
    emac_phy_write(dev, phy_addr, 0x00, (phy_val & ~(0x0800)));
    while (emac_phy_read(dev, phy_addr, 0x00) & (0x0800));

    /* Wait Auto negotiation success */
    while (!(emac_phy_read(dev, phy_addr, 0x01) & 0x0020))
    {
        if (i > 4)
        {
            rt_kprintf("Warning: Auto negotiation timeout!\n");
        }
        rt_hw_us_delay(500*1000);
        i++;
    }

    phy_val = emac_phy_read(dev, phy_addr, 0x00);

    reg_val = readl(dev->base_addr + REG_EMAC_BASIC_CTL0);
    if (phy_val & 0x0100)
    {
        reg_val |= 0x01;
    }
    else
    {
        reg_val &= ~0x01;
    }

    /* Default is 1000Mbps */
    reg_val &= ~0x0c;
    if (phy_val & 0x2000)
    {
        reg_val |= 0x0c;
    }
    else if (!(phy_val & 0x0040))
    {
        reg_val |= 0x08;
    }
    writel(reg_val, dev->base_addr + REG_EMAC_BASIC_CTL0);

#if 0
    /* just for debug loopback */
    phy_val = emac_phy_read(dev, phy_addr, 0x00);
    phy_val |= 0x4000;
    emac_phy_write(dev, phy_addr, 0x00, phy_val);
    phy_val = emac_phy_read(dev, phy_addr, 0x00);
    rt_kprintf("REG0: %08x\n", phy_val);

    /* MAC layer Loopback */
    reg_val = readl(dev->base_addr + REG_EMAC_BASIC_CTL0);
    reg_val |= 0x02;
    writel(reg_val, dev->base_addr + REG_EMAC_BASIC_CTL0);
#endif

    /* Read rgmii cfg1 */
    emac_phy_write(dev, phy_addr, 0x1E, 0xA003);
    rt_hw_us_delay(100 * 1000);
    phy_val = emac_phy_read(dev, phy_addr, 0x1F);
    rt_hw_us_delay(100 * 1000);

    phy_val &= ~(0xf << 0);
    phy_val |= (0x03 << 0);

    phy_val &= ~(0xf << 10);
    phy_val |= (0 << 10);

    /* Config phy tx/rx delay */
    emac_phy_write(dev, phy_addr, 0x1E, 0xA003);
    rt_hw_us_delay(100 * 1000);
    emac_phy_write(dev, phy_addr, 0x1F, phy_val);

    phy_val = emac_phy_read(dev, phy_addr, 0x00);
    rt_kprintf("emac   Speed: %dMbps, Mode: %s, Loopback: %s\n",
                ((phy_val & 0x0040) ? 1000 :
                ((phy_val & 0x2000) ? 100 : 10)),
                (phy_val & 0x0100)  ? "Full duplex" : "Half duplex",
                (phy_val & 0x4000)  ? "YES" : "NO");
}

static struct iomux_cfg mdc  = {.port=IO_PORTG,.pin=PIN_14,.mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg mdio = {.port=IO_PORTG,.pin=PIN_15,.mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg txd0 = {.port=IO_PORTG,.pin=PIN_4, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg txd1 = {.port=IO_PORTG,.pin=PIN_5, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg txd2 = {.port=IO_PORTG,.pin=PIN_6, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg txd3 = {.port=IO_PORTG,.pin=PIN_7, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg txctl= {.port=IO_PORTG,.pin=PIN_12,.mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg txclk= {.port=IO_PORTG,.pin=PIN_3, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg rxd0 = {.port=IO_PORTG,.pin=PIN_1, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg rxd1 = {.port=IO_PORTG,.pin=PIN_2, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg rxd2 = {.port=IO_PORTG,.pin=PIN_8, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg rxd3 = {.port=IO_PORTG,.pin=PIN_9, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg rxctl= {.port=IO_PORTG,.pin=PIN_0, .mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};
static struct iomux_cfg rxclk= {.port=IO_PORTG,.pin=PIN_10,.mux=IO_PERIPH_MUX4,.pull=IO_PULL_RESERVE};

struct emac_device emac0 =
{
    .base_addr = EMAC_BASE_ADDR,
    .irq_num   = EMAC_IRQ_NUM,
    .tx_completed = RT_FALSE,
    .rx_completed = RT_FALSE,
};

static const struct rt_device_ops emac_ops =
{
    .init = RT_NULL,
    .open = RT_NULL,
    .read = RT_NULL,
    .write = RT_NULL,
    .control = t113_emac_control,
};

static void phy_monitor_thread_entry(void *parameter)
{
    int current_status = 0;
    int old_status = 1;
    emac_dev_t dev = (emac_dev_t)parameter;

    while (1)
    {
        current_status = emac_phy_read(dev, 0x00, 0x01) & (1 << 2);
        if (current_status != old_status)
        {
            old_status = current_status;
            if (current_status == 0x04)
            {
                rt_kprintf("[%s]: Link is UP\n", dev->device.parent.parent.name);
                emac_phy_init(dev);
                eth_device_linkchange(&dev->device, RT_TRUE);
            }
            else
            {
                rt_kprintf("[%s]: Link is Down\n", dev->device.parent.parent.name);
                eth_device_linkchange(&dev->device, RT_FALSE);
            }
        }
        rt_thread_delay(500);
    }
}

static int t113_emac_init(void)
{
    rt_uint32_t reg_val = 0;
    rt_err_t ret = RT_EOK;
    struct eth_device *netif = RT_NULL;
    rt_thread_t tid = RT_NULL;

    /* iomux */
    iomux_set_sel(&mdc);
    iomux_set_sel(&mdio);
    iomux_set_sel(&txd0);
    iomux_set_sel(&txd1);
    iomux_set_sel(&txd2);
    iomux_set_sel(&txd3);
    iomux_set_sel(&txctl);
    iomux_set_sel(&txclk);
    iomux_set_sel(&rxd0);
    iomux_set_sel(&rxd1);
    iomux_set_sel(&rxd2);
    iomux_set_sel(&rxd3);
    iomux_set_sel(&rxctl);
    iomux_set_sel(&rxclk);

    tx_ring.tx_desc_pool = uncache_malloc_align((EMAC_DMA_TX_DESC_NUM * sizeof(struct dma_desc)), 64);
    if (tx_ring.tx_desc_pool == RT_NULL)
    {
        rt_kprintf("tx_desc_pool malloc err.\n");
        uncache_free(tx_ring.tx_desc_pool);
        return -RT_ERROR;
    }

    rx_ring.rx_desc_pool = uncache_malloc_align((EMAC_DMA_RX_DESC_NUM * sizeof(struct dma_desc)), 64);
    if (rx_ring.rx_desc_pool == RT_NULL)
    {
        rt_kprintf("rx_desc_pool malloc err.\n");
        uncache_free(rx_ring.rx_desc_pool);
        uncache_free(tx_ring.tx_desc_pool);
        return -RT_ERROR;
    }

    /* Reset all components */
    reg_val = readl(emac0.base_addr + REG_EMAC_BASIC_CTL1);
    reg_val |= 0x01;
    writel(reg_val, emac0.base_addr + REG_EMAC_BASIC_CTL1);
    while(readl(emac0.base_addr + REG_EMAC_BASIC_CTL1) & (0x01));

    /* Initialize core */
    reg_val = readl(emac0.base_addr + REG_EMAC_TX_CTL0);
    reg_val |= (3 << 30);   /* Enable transmit component &  Jabber Disable */
    writel(reg_val, emac0.base_addr + REG_EMAC_TX_CTL0);

    reg_val = readl(emac0.base_addr + REG_EMAC_TX_CTL1);
    /* Transmit COE type 2 cannot be done in cut-through mode. */
    reg_val |= (0x02);
    writel(reg_val, emac0.base_addr + REG_EMAC_TX_CTL1);

    reg_val = readl(emac0.base_addr + REG_EMAC_RX_CTL0);
    reg_val |= (0x0f << 28); /* Automatic Pad/CRC Stripping */
    writel(reg_val, emac0.base_addr + REG_EMAC_RX_CTL0);

    reg_val = readl(emac0.base_addr + REG_EMAC_RX_CTL1);
    reg_val |= (0x02);
    writel(reg_val, emac0.base_addr + REG_EMAC_RX_CTL1);

    /* GMAC frame filter */
    reg_val = readl(emac0.base_addr + REG_EMAC_RX_FRM_FLT);
    reg_val |= 0x00000001;
    writel(reg_val, emac0.base_addr + REG_EMAC_RX_FRM_FLT);

    /* Burst should be 8 */
    reg_val = readl(emac0.base_addr + REG_EMAC_BASIC_CTL1);
    reg_val |= (8 << 24);
    writel(reg_val, emac0.base_addr + REG_EMAC_BASIC_CTL1);

    /* Seth hardware address */
    emac_write_hwaddr(&emac0, mac_addr);

    reg_val = readl(emac0.base_addr + REG_EMAC_RX_CTL1);
    reg_val |= 0xC0000000;
    writel(reg_val, emac0.base_addr + REG_EMAC_RX_CTL1);

    rt_hw_interrupt_install(emac0.irq_num, emac_irq_handle, &emac0, "emac");
    rt_hw_interrupt_umask(emac0.irq_num);

    emac_dma_tx_desc_init();
    emac_fill_tx_desc(&emac0);
    emac_dma_rx_desc_init();
    emac_fill_rx_desc(&emac0);

    /* Enable recv interrupt of dma */
    writel((0x1 << 13), emac0.base_addr + REG_EMAC_INT_EN);

    netif                   = &emac0.device;
    netif->parent.type      = RT_Device_Class_NetIf;
    netif->parent.ops       = &emac_ops;
    netif->parent.user_data = RT_NULL;
    netif->eth_tx           = t113_emac_tx;
    netif->eth_rx           = t113_emac_rx;
    ret = eth_device_init(netif, "e0");
    if (ret != RT_EOK)
    {
        rt_kprintf("%s netif init fail: %d", "e0", ret);
        uncache_free(rx_ring.rx_desc_pool);
        uncache_free(tx_ring.tx_desc_pool);
        return (-RT_ERROR);
    }

    tid = rt_thread_create("phy", phy_monitor_thread_entry, &emac0, 2048, 16, 10);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }

    return RT_EOK;
}
INIT_DEVICE_EXPORT(t113_emac_init);

void dump_tx_desc(void)
{
    int i = 0;

    for (i = 0; i < EMAC_DMA_TX_DESC_NUM; i++)
    {
        rt_kprintf("\n%d\n", i);
        rt_kprintf("desc0 : %p\n", tx_ring.tx_desc_pool[i].desc0);
        rt_kprintf("desc1 : %p\n", tx_ring.tx_desc_pool[i].desc1);
        rt_kprintf("desc2 : %p\n", tx_ring.tx_desc_pool[i].desc2);
        rt_kprintf("desc3 : %p\n", tx_ring.tx_desc_pool[i].desc3);
    }

    rt_kprintf("interrupt_status : %p\n", readl(emac0.base_addr + REG_EMAC_INT_STA));
    rt_kprintf("tx_control0      : %p\n", readl(emac0.base_addr + REG_EMAC_TX_CTL0));
    rt_kprintf("tx_control1      : %p\n", readl(emac0.base_addr + REG_EMAC_TX_CTL1));
    rt_kprintf("tx_status        : %p\n", readl(emac0.base_addr + REG_EMAC_TX_DMA_STA));
    rt_kprintf("tx_current desc  : %p\n", readl(emac0.base_addr + REG_EMAC_TX_CUR_DESC));
    rt_kprintf("tx_current buf   : %p\n", readl(emac0.base_addr + REG_EMAC_TX_CUR_BUF));
    // writel(readl(emac0.base_addr + REG_EMAC_INT_STA), emac0.base_addr + REG_EMAC_INT_STA);
}
MSH_CMD_EXPORT(dump_tx_desc, dump_tx_desc);

void dump_rx_desc(void)
{
    int i = 0;

    for (i = 0; i < EMAC_DMA_RX_DESC_NUM; i++)
    {
        rt_kprintf("\n%d\n", i);
        rt_kprintf("desc0 : %p\n", rx_ring.rx_desc_pool[i].desc0);
        rt_kprintf("desc1 : %p\n", rx_ring.rx_desc_pool[i].desc1);
        rt_kprintf("desc2 : %p\n", rx_ring.rx_desc_pool[i].desc2);
        rt_kprintf("desc3 : %p\n", rx_ring.rx_desc_pool[i].desc3);
    }

    rt_kprintf("interrupt_status : %p\n", readl(emac0.base_addr + REG_EMAC_INT_STA));
    rt_kprintf("rx_control0 :      %p\n", readl(emac0.base_addr + REG_EMAC_RX_CTL0));
    rt_kprintf("rx_control1 :      %p\n", readl(emac0.base_addr + REG_EMAC_RX_CTL1));
    rt_kprintf("rx_status        : %p\n", readl(emac0.base_addr + REG_EMAC_RX_DMA_STA));
    rt_kprintf("rx_current desc  : %p\n", readl(emac0.base_addr + REG_EMAC_RX_CUR_DESC));
    rt_kprintf("rx_current buf   : %p\n", readl(emac0.base_addr + REG_EMAC_RX_CUR_BUF));
    // writel(readl(emac0.base_addr + REG_EMAC_INT_STA), emac0.base_addr + REG_EMAC_INT_STA);
}
MSH_CMD_EXPORT(dump_rx_desc, dump_rx_desc);
