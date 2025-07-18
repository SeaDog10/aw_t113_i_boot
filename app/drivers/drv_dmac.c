#include <drv_dmac.h>
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "[drv.dmac]"
#define BSP_ENBALE_DMAC_DEBUG 1

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
static dma_source_t dma_channel_source[T113_DMAC_CHANNEL_MAX];
static dma_desc_t   dma_channel_desc[T113_DMAC_CHANNEL_MAX] __attribute__((aligned(64)));

void dma_init(void)
{
    int i = 0;
    dma_reg_t *const dma_reg = (dma_reg_t *)DMAC_BASE_ADDR;

    LOG_D("DMA: init");

    if (dma_init_ok > 0)
    {
        LOG_D("DMA: already init");
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
    commit_para |= (dma_set->data_block_size & 0xff) << 8;

    desc->commit_para   = commit_para;
    desc->config        = *(volatile rt_uint32_t *)channel_addr;

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

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, (void *)desc,  sizeof(dma_desc_t));
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, (void *)saddr, bytes);

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

static rt_uint8_t src_buf [512 * 1024]  = {0};
static rt_uint8_t desc_buf[512 * 1024]  = {0};

void dma_mem_to_mem_test(void)
{
    rt_uint32_t     *src_addr = (rt_uint32_t *)src_buf;
    rt_uint32_t     *dst_addr = (rt_uint32_t *)desc_buf;
    rt_uint32_t     len       = 512 * 1024;
    dma_set_t       dma_set   = {0};
    rt_uint32_t     hdma = 0, st  = 0;
    rt_uint32_t     i, valid;
    rt_tick_t       timeout = 100;

    dma_init();

    len = RT_ALIGN(len, 4);
    LOG_D("DMA: test 0x%08x ====> 0x%08x, len %dKB", (rt_uint32_t)src_addr, (rt_uint32_t)dst_addr, (len / 1024));

    /* dma */
    dma_set.loop_mode       = 0;
    dma_set.wait_cyc        = 8;
    dma_set.data_block_size = 1 * 32 / 8;
    /* channel config (from dram to dram)*/
    dma_set.channel_cfg.src_drq_type     = DMAC_CFG_TYPE_DRAM; // dram
    dma_set.channel_cfg.src_burst_length = DMAC_CFG_SRC_4_BURST;
    dma_set.channel_cfg.src_addr_mode    = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
    dma_set.channel_cfg.src_data_width   = DMAC_CFG_SRC_DATA_WIDTH_16BIT;
    dma_set.channel_cfg.reserved0        = 0;

    dma_set.channel_cfg.dst_drq_type     = DMAC_CFG_TYPE_DRAM; // dram
    dma_set.channel_cfg.dst_burst_length = DMAC_CFG_DEST_4_BURST;
    dma_set.channel_cfg.dst_addr_mode    = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
    dma_set.channel_cfg.dst_data_width   = DMAC_CFG_DEST_DATA_WIDTH_16BIT;
    dma_set.channel_cfg.reserved1        = 0;

    hdma = dma_request(0);
    if (!hdma)
    {
        LOG_E("DMA: can't request dma");
        return;
    }

    dma_setting(hdma, &dma_set);

    // prepare data
    for (i = 0; i < (len / 4); i += 4)
    {
        src_addr[i]     = i;
        src_addr[i + 1] = i + 1;
        src_addr[i + 2] = i + 2;
        src_addr[i + 3] = i + 3;
    }

    timeout = rt_tick_get();

    dma_start(hdma, (rt_uint32_t)src_addr, (rt_uint32_t)dst_addr, len);
    st = dma_querystatus(hdma);

    while (((rt_tick_get() - timeout) < 10000) && st)
    {
        st = dma_querystatus(hdma);
        rt_thread_mdelay(1);
    }

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, dst_addr, 1);

    if (st)
    {
        LOG_E("DMA: test timeout!");
        dma_stop(hdma);
        dma_release(hdma);
        return;
    }
    else
    {
        valid = 1;
        // Check data is valid
        for (i = 0; i < (len / 4); i += 4)
        {
            if (dst_addr[i] != i || dst_addr[i + 1] != i + 1 || dst_addr[i + 2] != i + 2 || dst_addr[i + 3] != i + 3)
            {
                valid = 0;
                break;
            }
        }
        if (valid)
        {
            LOG_D("DMA: test OK in %lums", (rt_tick_get() - timeout));
        }
        else
        {
            LOG_E("DMA: test check failed at %d bytes", i);
        }
    }

    dma_stop(hdma);
    dma_release(hdma);
}
MSH_CMD_EXPORT(dma_mem_to_mem_test, dma_mem_to_mem_test);

typedef struct dmac_device
{
    struct rt_device parent;
    rt_uint32_t base;
    rt_bool_t init_ok;
} *dmac_dev_t;

static rt_err_t t113_dmac_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t t113_dmac_control(rt_device_t dev, int cmd, void *args)
{
    return RT_EOK;
}

static struct rt_device_ops dmac_ops =
{
    .init    = t113_dmac_init,
    .open    = RT_NULL,
    .close   = RT_NULL,
    .read    = RT_NULL,
    .write   = RT_NULL,
    .control = t113_dmac_control,
};

int rt_hw_dmac_init(void)
{

}
