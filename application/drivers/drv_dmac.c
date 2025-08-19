#include <drv_dmac.h>
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "[drv.dmac]"
#define BSP_ENBALE_DMAC_DEBUG 0

#define DBG_ENABLE
#if (BSP_ENBALE_DMAC_DEBUG == 1)
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_WARNING
#endif
#define DBG_COLOR
#include <rtdbg.h>

typedef struct dma_channel_reg
{
    volatile rt_uint32_t enable;
    volatile rt_uint32_t pause;
    volatile rt_uint32_t desc_addr;
    volatile rt_uint32_t config;
    volatile rt_uint32_t cur_src_addr;
    volatile rt_uint32_t cur_dst_addr;
    volatile rt_uint32_t left_bytes;
    volatile rt_uint32_t parameters;
    volatile rt_uint32_t mode;
    volatile rt_uint32_t fdesc_addr;
    volatile rt_uint32_t pkg_num;
    volatile rt_uint32_t res[5];
} dma_channel_reg_t;

typedef struct dma_reg
{
    volatile rt_uint32_t irq_en0;                     /* 0x0 dma irq enable register 0 */
    volatile rt_uint32_t irq_en1;                     /* 0x4 dma irq enable register 1 */
    volatile rt_uint32_t reserved0[2];
    volatile rt_uint32_t irq_pending0;                /* 0x10 dma irq pending register 0 */
    volatile rt_uint32_t irq_pending1;                /* 0x14 dma irq pending register 1 */
    volatile rt_uint32_t reserved1[2];
    volatile rt_uint32_t security;                    /* 0x20 dma security register */
    volatile rt_uint32_t reserved3[1];
    volatile rt_uint32_t auto_gate;                   /* 0x28 dma auto gating register */
    volatile rt_uint32_t reserved4[1];
    volatile rt_uint32_t status;                      /* 0x30 dma status register */
    volatile rt_uint32_t reserved5[3];
    volatile rt_uint32_t version;                     /* 0x40 dma Version register */
    volatile rt_uint32_t reserved6[47];
    dma_channel_reg_t channel[T113_DMAC_CHANNEL_MAX]; /* 0x100 dma channel register */
} dma_reg_t;

typedef struct dma_descript
{
    volatile rt_uint32_t config;
    volatile rt_uint32_t source_addr;
    volatile rt_uint32_t dest_addr;
    volatile rt_uint32_t byte_count;
    volatile rt_uint32_t commit_para;
    volatile rt_uint32_t link;
    volatile rt_uint32_t reserved[2];
} dma_desc_t;

typedef struct {
    rt_uint32_t         used;
    rt_uint32_t         channel_count;
    dma_channel_reg_t   *channel;
    rt_uint32_t         reserved;
    dma_desc_t          *desc;
} dma_source_t;

static int          dma_int_cnt = 0;
static int          dma_init_ok = -1;
static dma_source_t dma_channel_source[T113_DMAC_CHANNEL_MAX] __attribute__((aligned(512)));
static dma_desc_t   dma_channel_desc[T113_DMAC_CHANNEL_MAX]   __attribute__((aligned(512)));
static void (*dma_channel_handler[T113_DMAC_CHANNEL_MAX])(void);

void dmac_irq_handler(int vector, void *param)
{
    dma_reg_t *const dma_reg = (dma_reg_t *)DMAC_BASE_ADDR;
    rt_uint32_t reg0 = dma_reg->irq_pending0;
    rt_uint32_t reg1 = dma_reg->irq_pending1;
    rt_uint32_t mask;
    int ch = 0;

    for (ch = 0; ch < 8; ch++)
    {
        mask = 0x7u << (ch * 4);
        if (reg0 & mask)
        {
            if (dma_channel_handler[ch])
            {
                dma_channel_handler[ch]();
            }
            dma_reg->irq_pending0 = mask;
        }
    }

    for (ch = 0; ch < 8; ch++)
    {
        mask = 0x7u << (ch * 4);
        if (reg1 & mask)
        {
            if (dma_channel_handler[ch + 8])
            {
                dma_channel_handler[ch + 8]();
            }
            dma_reg->irq_pending1 = mask;
        }
    }
}

void dma_init(void)
{
    int i = 0;
    dma_reg_t *const dma_reg = (dma_reg_t *)DMAC_BASE_ADDR;

    if (dma_init_ok > 0)
    {
        return;
    }

    dma_reg->irq_en0 = 0;
    dma_reg->irq_en1 = 0;

    dma_reg->irq_pending0 = 0xffffffff;
    dma_reg->irq_pending1 = 0xffffffff;

    /* auto MCLK gating  disable */
    dma_reg->auto_gate &= ~(0x7 << 0);
    dma_reg->auto_gate |= 0x7 << 0;

    rt_memset((void *)dma_channel_source, 0, T113_DMAC_CHANNEL_MAX * sizeof(dma_source_t));

    for (i = 0; i < T113_DMAC_CHANNEL_MAX; i++)
    {
        dma_channel_source[i].used      = 0;
        dma_channel_source[i].channel   = &(dma_reg->channel[i]);
        dma_channel_source[i].desc      = &dma_channel_desc[i];
        dma_channel_handler[i]          = RT_NULL;
    }

    dma_int_cnt = 0;
    dma_init_ok = 1;
}

void dma_exit(void)
{
    int i = 0;
    dma_reg_t *dma_reg = (dma_reg_t *)DMAC_BASE_ADDR;

    /* free dma channel if other module not free it */
    for (i = 0; i < T113_DMAC_CHANNEL_MAX; i++)
    {
        if (dma_channel_source[i].used == 1)
        {
            dma_channel_source[i].channel->enable = 0;
            dma_channel_source[i].used            = 0;
        }
    }

    /* close dma clock when dma exit */
    dma_reg->auto_gate &= ~(1 << DMA_GATING_OFS | 1 << DMA_RST_OFS);

    dma_reg->irq_en0 = 0;
    dma_reg->irq_en1 = 0;

    dma_reg->irq_pending0 = 0xffffffff;
    dma_reg->irq_pending1 = 0xffffffff;

    dma_init_ok--;
}

rt_uint32_t dma_request_from_last(rt_uint32_t dmatype)
{
    int i = 0;

    for (i = T113_DMAC_CHANNEL_MAX - 1; i >= 0; i--)
    {
        if (dma_channel_source[i].used == 0)
        {
            dma_channel_source[i].used          = 1;
            dma_channel_source[i].channel_count = i;
            return (rt_uint32_t)&dma_channel_source[i];
        }
    }

    LOG_E("dma_request_from_last fail.");
    return 0;
}

rt_uint32_t dma_request(rt_uint32_t dmatype)
{
    int i = 0;

    for (i = 0; i < T113_DMAC_CHANNEL_MAX; i++)
    {
        if (dma_channel_source[i].used == 0)
        {
            dma_channel_source[i].used          = 1;
            dma_channel_source[i].channel_count = i;
            LOG_D("DMA: provide channel %u", i);
            return (rt_uint32_t)&dma_channel_source[i];
        }
    }

    LOG_E("dma_request fail");
    return 0;
}

int dma_release(rt_uint32_t hdma)
{
    dma_source_t *dma_source = (dma_source_t *)hdma;

    if (!dma_source->used)
    {
        LOG_E("dma_release dma_source is not used");
        return -1;
    }

    dma_source->used = 0;

    return 0;
}

int dma_setting(rt_uint32_t hdma, dma_set_t *cfg)
{
    rt_uint32_t  commit_para;
    dma_set_t    *dma_set       = cfg;
    dma_source_t *dma_source    = (dma_source_t *)hdma;
    dma_desc_t   *desc          = dma_source->desc;
    rt_uint32_t  channel_addr   = (rt_uint32_t)(&(dma_set->channel_cfg));
    dma_reg_t *const dma_reg = (dma_reg_t *)DMAC_BASE_ADDR;

    if (!dma_source->used)
    {
        LOG_E("dma_setting dma_source is not be used");
        return -1;
    }

    if (dma_set->loop_mode)
    {
        desc->link = (rt_uint32_t)(&dma_source->desc);
    }
    else
    {
        desc->link = SUNXI_DMA_LINK_NULL;
    }

    commit_para  = (dma_set->wait_cyc & 0xff);

    desc->commit_para   = commit_para;
    desc->config        = *(volatile rt_uint32_t *)channel_addr;

    if (dma_set->dma_callback)
    {
        if (dma_source->channel_count < 8)
        {
            dma_reg->irq_en0 |= (1 << ((dma_source->channel_count * 4) + dma_set->callback_type));
        }
        else
        {
            dma_reg->irq_en1 |= (1 << (((dma_source->channel_count - 8) * 4) + dma_set->callback_type));
        }

        dma_channel_handler[dma_source->channel_count] = dma_set->dma_callback;
    }

    return 0;
}

int dma_start(rt_uint32_t hdma, rt_uint32_t saddr, rt_uint32_t daddr, rt_uint32_t bytes)
{
    dma_source_t      *dma_source = (dma_source_t *)hdma;
    dma_desc_t        *desc       = dma_source->desc;
    dma_channel_reg_t *channel    = dma_source->channel;

    if (!dma_source->used)
    {
        LOG_E("dma_start dma_source is not be used");
        return -1;
    }

    /*config desc */
    desc->source_addr = saddr;
    desc->dest_addr   = daddr;
    desc->byte_count  = bytes;

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,      (void *)saddr, bytes);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, (void *)daddr, bytes);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,      (void *)desc,  sizeof(dma_desc_t));

    /* start dma */
    channel->desc_addr = (rt_uint32_t)desc;
    channel->enable    = 1;

    return 0;
}

int dma_stop(rt_uint32_t hdma)
{
    dma_source_t      *dma_source = (dma_source_t *)hdma;
    dma_channel_reg_t *channel    = dma_source->channel;

    if (!dma_source->used)
    {
        LOG_E("dma_stop dma_source is not be used");
        return -1;
    }

    channel->enable = 0;

    return 0;
}

int dma_querystatus(rt_uint32_t hdma)
{
    rt_uint32_t channel_count = 0;
    dma_source_t *dma_source  = (dma_source_t *)hdma;
    dma_reg_t *dma_reg        = (dma_reg_t *)DMAC_BASE_ADDR;

    if (!dma_source->used)
    {
        LOG_E("dma_querystatus dma_source is not be used");
        return -1;
    }

    channel_count = dma_source->channel_count;

    return (dma_reg->status >> channel_count) & 0x01;
}

int rt_hw_dma_init(void)
{
    int ret = 0;

    dma_init();

    rt_hw_interrupt_install(DMAC_IRQ_NUM, dmac_irq_handler, RT_NULL, "dmac");
    rt_hw_interrupt_umask(DMAC_IRQ_NUM);

    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_dma_init);

#if 0
#define DMA_TEST_LEN 32         /* byte */

#define DMA_TEST_SRC_ADDR        (0x40000000)
#define DMA_TEST_DEST_ADDR       (0x40080000)

static volatile rt_uint32_t src_buffer[DMA_TEST_LEN] __attribute__((aligned(512))) = {0};
static volatile rt_uint32_t dest_buffer[DMA_TEST_LEN] __attribute__((aligned(512))) = {0};

static struct rt_semaphore dma_sem = {0};

void dma_test_callback(void)
{
    rt_sem_release(&dma_sem);
}

void dma_mem_to_mem_test(void)
{
    int ret = 0;
    static int sem_init = 0;
    dma_set_t       dma_set   = {0};
    rt_uint32_t     hdma = 0;
    rt_uint32_t     i;
    rt_tick_t       timeout = 0;
    rt_uint32_t      *src_addr  =  (rt_uint32_t *)src_buffer;
    rt_uint32_t      *dest_addr = (rt_uint32_t *)dest_buffer;
    rt_uint32_t     len         = DMA_TEST_LEN * 4;

    if (sem_init == 0)
    {
        rt_sem_init(&dma_sem, "dma", 0, RT_IPC_FLAG_PRIO);
        sem_init = 1;
    }

    len = RT_ALIGN(len, 4);
    rt_kprintf("DMA: test 0x%08x ====> 0x%08x, len %dB\n", (rt_uint32_t)src_addr, (rt_uint32_t)dest_addr, (len));

    dma_set.loop_mode       = 0;
    dma_set.wait_cyc        = 8;

    dma_set.channel_cfg.src_drq_type     = DMAC_CFG_TYPE_DRAM; // dram
    dma_set.channel_cfg.src_block_size   = DMAC_CFG_SRC_1_BURST;
    dma_set.channel_cfg.src_addr_mode    = DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE;
    dma_set.channel_cfg.src_data_width   = DMAC_CFG_SRC_DATA_WIDTH_32BIT;
    dma_set.channel_cfg.reserved0        = 0;

    dma_set.channel_cfg.dst_drq_type     = DMAC_CFG_TYPE_DRAM; // dram
    dma_set.channel_cfg.dst_block_size   = DMAC_CFG_DEST_1_BURST;
    dma_set.channel_cfg.dst_addr_mode    = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
    dma_set.channel_cfg.dst_data_width   = DMAC_CFG_DEST_DATA_WIDTH_32BIT;
    dma_set.channel_cfg.reserved1        = 0;

    dma_set.dma_callback = dma_test_callback;
    dma_set.callback_type = DMA_QUEUE_END_INT;

    hdma = dma_request(0);
    if (!hdma)
    {
        LOG_E("DMA: can't request dma");
        return;
    }

    dma_setting(hdma, &dma_set);

    rt_memset(src_addr, 0x55, len);
    rt_memset(dest_addr, 0, len);

    for (i = 0; i < (DMA_TEST_LEN); i++)
    {
        rt_kprintf("src_addr[%d] %d\n", i, src_addr[i]);
    }

    timeout = rt_tick_get();

    dma_start(hdma, (rt_uint32_t)src_addr, (rt_uint32_t)dest_addr, len);

    LOG_D("REG_DMAC_EN[0]          0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_EN(0)));
    LOG_D("REG_DMAC_PAU[0]         0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_PAU(0)));
    LOG_D("REG_DMAC_DESC_ADDR[0]   0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_DESC_ADDR(0)));
    LOG_D("REG_DMAC_CFG[0]         0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_CFG(0)));
    LOG_D("REG_DMAC_CUR_SRC[0]     0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_CUR_SRC(0)));
    LOG_D("REG_DMAC_CUR_DEST[0]    0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_CUR_DEST(0)));
    LOG_D("REG_DMAC_BCNT_LEFT[0]   0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_BCNT_LEFT(0)));
    LOG_D("REG_DMAC_PARA[0]        0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_PARA(0)));
    LOG_D("REG_DMAC_MODE[0]        0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_MODE(0)));
    LOG_D("REG_DMAC_FDESC_ADDR[0]  0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_FDESC_ADDR(0)));
    LOG_D("REG_DMAC_PKG_NUM[0]     0x%08x", readl(DMAC_BASE_ADDR + REG_DMAC_PKG_NUM(0)));

    ret = rt_sem_take(&dma_sem, 2000);

    if (ret != RT_EOK)
    {
        LOG_E("DMA: test timeout!");
        dma_stop(hdma);
        dma_release(hdma);
        return;
    }
    else
    {
        rt_kprintf("DMA: test in %dms\n", (rt_tick_get() - timeout));

        for (i = 0; i < (DMA_TEST_LEN); i++)
        {
            rt_kprintf("dest_addr[%d] %d\n", i, dest_addr[i]);
        }
    }

    dma_stop(hdma);
    dma_release(hdma);
}
MSH_CMD_EXPORT(dma_mem_to_mem_test, dma_mem_to_mem_test);
#endif