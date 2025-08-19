#include <board.h>
#include <drv_smhc.h>
#include <drv_iomux.h>
#include <drv_clk.h>

#define DBG_TAG "[drv.smhc]"
#define BSP_ENBALE_SMHC_DEBUG 0

#define DBG_ENABLE
#if (BSP_ENBALE_SMHC_DEBUG == 1)
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_WARNING
#endif
#define DBG_COLOR
#include <rtdbg.h>

#define SMHC_SOURCE_CLK     200000000

struct sdmmc_xfe_des
{
    rt_uint32_t             size;    /* block size  */
    rt_uint32_t             num;     /* block num   */
    rt_uint8_t              *buff;   /* buff addr   */
    rt_uint32_t             flag;    /* write or read or stream */
};

struct sdmmc_flag
{
    volatile rt_uint32_t    risr;
    volatile rt_uint32_t    idst;
};

typedef struct smhc_dev
{
    rt_uint32_t             base;
    rt_uint32_t             irq;
    rt_uint32_t             id;
    struct iomux_cfg        smhc_cmd;
    struct iomux_cfg        smhc_clk;
    struct iomux_cfg        smhc_d0;
    struct iomux_cfg        smhc_d1;
    struct iomux_cfg        smhc_d2;
    struct iomux_cfg        smhc_d3;
    struct iomux_cfg        smhc_det;
    struct rt_semaphore     smhc_sem;
    struct sdmmc_xfe_des    xfe;
    struct sdmmc_flag       flag;
    struct rt_mmcsd_host    *host;
    struct rt_mmcsd_req     *req;
}*smhc_dev_t;

static int sdmmc_reg_dump(void);

static void smhc_interrupt_handle(int irqno, void *param)
{
    rt_uint32_t risr = 0;
    rt_uint32_t idst = 0;
    rt_uint32_t status = 0;
    smhc_dev_t dev = (smhc_dev_t)param;
    struct rt_mmcsd_data *data = dev->req->cmd->data;

    risr = readl(dev->base + REG_SMHC_RINTSTS);
    idst = readl(dev->base + REG_SMHC_DST);

    writel((readl(dev->base + REG_SMHC_INTMASK) & risr), dev->base + REG_SMHC_RINTSTS);
    writel((readl(dev->base + REG_SMHC_IDIE) & idst), dev->base + REG_SMHC_DST);

    dev->flag.risr |= risr;
    dev->flag.idst |= idst;

    if (data)
    {
        int done = 0;

        status = dev->flag.risr | readl(dev->base + REG_SMHC_RINTSTS);

        if (data->blks > 1)
        {
            done = status & (1 << 14);
        }
        else
        {
            done = status & (1 << 3);
        }

        if (done)
        {
            if (rt_sem_release(&dev->smhc_sem) != RT_EOK)
            {
                LOG_E("sem release failed %d");
            }
        }
    }
    else
    {
        if (rt_sem_release(&dev->smhc_sem) != RT_EOK)
        {
            LOG_E("sem release failed %d");
        }
    }
}

static int sdmmc_update_clk(smhc_dev_t dev)
{
    rt_uint32_t timeout = 2000000;
    rt_uint32_t regval = 0;

    regval = (1 << 13) | (1 << 21) | (1 << 31);
    writel(regval, dev->base + REG_SMHC_CMD);

    while((readl(dev->base + REG_SMHC_CMD) & (1 << 31)) && (--timeout));
    if (!timeout)
    {
        return -RT_ERROR;
    }

    writel(readl(dev->base + REG_SMHC_RINTSTS), dev->base + REG_SMHC_RINTSTS);

    return RT_EOK;
}

static rt_err_t sdmmc_config_clock(smhc_dev_t dev, rt_uint32_t clk)
{
    rt_uint32_t regval = 0;

    /* disable card clock */
    regval = readl(dev->base + REG_SMHC_CLKDIV);
    regval &= ~(1 << 16);
    writel(regval, dev->base + REG_SMHC_CLKDIV);

    if (sdmmc_update_clk(dev) != RT_EOK)
    {
        LOG_E("clk update fail line:%d\n", __LINE__);
        return -RT_ERROR;
    }

    init_smhc0_clk(clk);

    regval = readl(dev->base + REG_SMHC_CLKDIV);
    regval |= (1 << 31);
    regval &= ~(0xff);
    writel(regval, dev->base + REG_SMHC_CLKDIV);

    regval = readl(dev->base + REG_SMHC_CLKDIV);
    regval |= (1 << 16);
    writel(regval, dev->base + REG_SMHC_CLKDIV);

    if(sdmmc_update_clk(dev) != RT_EOK)
    {
        LOG_E("clk update fail line:%d\n", __LINE__);
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t sdmmc_set_clk_width(smhc_dev_t dev, rt_uint32_t clk, rt_uint32_t bus_width)
{
    LOG_D("smhc set io bus width:(2^%d) clock:%d", bus_width, clk);

    /* change clock */
    if (clk && (sdmmc_config_clock(dev, clk) != RT_EOK))
    {
        LOG_E("update clock failed");
        return -RT_ERROR;
    }

    /* Change bus width */
    if (bus_width == MMCSD_BUS_WIDTH_8)
    {
        writel(2, dev->base + REG_SMHC_BUSWID);
    }
    else if (bus_width == MMCSD_BUS_WIDTH_4)
    {
        writel(1, dev->base + REG_SMHC_BUSWID);
    }
    else
    {
        writel(0, dev->base + REG_SMHC_BUSWID);
    }

    return RT_EOK;
}

static rt_err_t sdmmc_trans_data(smhc_dev_t dev, struct sdmmc_xfe_des *xfe)
{
    rt_uint32_t i = 0;
    rt_uint32_t byte_cnt = xfe->size * xfe->num;
    rt_uint32_t *buff = (rt_uint32_t *)(xfe->buff);
    volatile rt_uint32_t timeout = 2000000;

    if (xfe->flag == MMC_DATA_WRITE)
    {
        for (i = 0; i < (byte_cnt >> 2); i++)
        {
            while(--timeout && (readl(dev->base + REG_SMHC_STATUS) & (1 << 3)));

            if (timeout <= 0)
            {
                LOG_E("write data by cpu failed status:0x%08x", readl(dev->base + REG_SMHC_STATUS));
                return -RT_ERROR;
            }
            writel(buff[i], dev->base + REG_SMHC_FIFO);
            timeout = 2000000;
        }
    }
    else
    {
        for (i = 0; i < (byte_cnt >> 2); i++)
        {
            while(--timeout && (readl(dev->base + REG_SMHC_STATUS) & (1 << 2)));

            if (timeout <= 0)
            {
                LOG_E("write data by cpu failed status:0x%08x", readl(dev->base + REG_SMHC_STATUS));
                return -RT_ERROR;
            }
            buff[i] = readl(dev->base + REG_SMHC_FIFO);
            timeout = 2000000;
        }
    }

    return RT_EOK;
}

static int sdmmc_send_cmd(struct rt_mmcsd_host *host, struct rt_mmcsd_cmd *cmd)
{
    unsigned int cmdval = 0x80000000;
    signed int timeout = 0;
    int err = 0;
    unsigned int status  = 0;
    unsigned int bytecnt = 0;
    struct rt_mmcsd_data *data = cmd->data;
    smhc_dev_t dev = (smhc_dev_t)host->private_data;
    rt_uint32_t regval = 0;

    timeout = 5000 * 1000;

    status = readl(dev->base + REG_SMHC_STATUS);
    while (status & (1 << 9))
    {
        LOG_D("note: check card busy");

        status = readl(dev->base + REG_SMHC_STATUS);
        if (!timeout--)
        {
            err = (-RT_ERROR);
            LOG_E("smhc cmd12 busy timeout data:0x%08x", readl(dev->base + REG_SMHC_STATUS));
            return err;
        }
    }

    if (!cmd->cmd_code)
        cmdval |= (1 << 15);
    if (resp_type(cmd) != RESP_NONE)
        cmdval |= (1 << 6);
    if (resp_type(cmd) == RESP_R2)
        cmdval |= (1 << 7);
    if ((resp_type(cmd) != RESP_R3) && (resp_type(cmd) != RESP_R4))
        cmdval |= (1 << 8);

    if (data)
    {
        cmdval |= (1 << 9) | (1 << 13);
        if (data->flags & DATA_DIR_WRITE)
            cmdval |= (1 << 10);
        if (data->blks > 1)
            cmdval |= (1 << 12);
        writel(data->blksize, dev->base + REG_SMHC_BLKSIZ);
        bytecnt = data->blksize * data->blks;
        writel(bytecnt, dev->base + REG_SMHC_BYTCNT);
    }

    LOG_D("cmd %d(0x%08x), arg 0x%08x", cmd->cmd_code, cmdval | cmd->cmd_code, cmd->arg);

    writel(cmd->arg, dev->base + REG_SMHC_CMDARG);

    if (!data)
    {
        writel((cmdval | cmd->cmd_code), dev->base + REG_SMHC_CMD);
        regval = readl(dev->base + REG_SMHC_INTMASK);
        regval |= (0x1 << 2);
        writel(regval, dev->base + REG_SMHC_INTMASK);
    }

    if (data)
    {
        LOG_D("smhc trans data %d bytes addr:0x%08x", bytecnt, data);
        regval = readl(dev->base + REG_SMHC_CTRL);
        regval |= (1 << 31);
        writel(regval, dev->base + REG_SMHC_CTRL);
        writel((cmdval | cmd->cmd_code), dev->base + REG_SMHC_CMD);
        sdmmc_trans_data(dev, &dev->xfe);

        if (data->blks > 1)
        {
            regval = readl(dev->base + REG_SMHC_INTMASK);
            regval |= (1 << 14);
            writel(regval, dev->base + REG_SMHC_INTMASK);
        }
        else
        {
            regval = readl(dev->base + REG_SMHC_INTMASK);
            regval |= (1 << 3);
            writel(regval, dev->base + REG_SMHC_INTMASK);
        }
    }

    regval = readl(dev->base + REG_SMHC_INTMASK);
    regval |= 0xbfc2;
    writel(regval, dev->base + REG_SMHC_INTMASK);

    timeout = 2000;

    if (rt_sem_take(&dev->smhc_sem, timeout) != RT_EOK)
    {
        err = (readl(dev->base + REG_SMHC_RINTSTS) | dev->flag.risr) & 0xbfc2;
        LOG_I("sem take error timeout:%d",timeout);
        goto out;
    }

    err = (readl(dev->base + REG_SMHC_RINTSTS) | dev->flag.risr) & 0xbfc2;
    if (err)
    {
        cmd->err = -RT_ETIMEOUT;
        LOG_I("time out");
        goto out;
    }

    if (resp_type(cmd) == RESP_R2)
    {
        cmd->resp[3] = readl(dev->base + REG_SMHC_RESP0);
        cmd->resp[2] = readl(dev->base + REG_SMHC_RESP1);
        cmd->resp[1] = readl(dev->base + REG_SMHC_RESP2);
        cmd->resp[0] = readl(dev->base + REG_SMHC_RESP3);
        LOG_D("smhc resp 0x%08x 0x%08x 0x%08x 0x%08x", cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
    }
    else
    {
        cmd->resp[0] = readl(dev->base + REG_SMHC_RESP0);
        LOG_D("smhc resp 0x%08x", cmd->resp[0]);
    }

out:
    if (err)
    {
        sdmmc_reg_dump();
    }

    if (err)
    {
        if (data && (data->flags & DATA_DIR_READ) && (bytecnt == 512))
        {
            regval = readl(dev->base + REG_SMHC_CTRL);
            regval |= (1 << 31);
            writel(regval, dev->base + REG_SMHC_CTRL);

            timeout = 1000;
            LOG_D("Read remain data");
            while (readl(dev->base + REG_SMHC_TBBCN) < 512)
            {
                unsigned int tmp = readl(dev->base + REG_SMHC_FIFO);
                tmp = tmp;
                LOG_D("Read data 0x%08x, bbcr 0x%04x", tmp, readl(dev->base + REG_SMHC_TBBCN));
                if (!(timeout--))
                {
                    LOG_E("Read remain data timeout");
                    break;
                }
            }
        }

        writel(0x7, dev->base + REG_SMHC_CTRL);
        while (readl(dev->base + REG_SMHC_CTRL) & 0x7);

        sdmmc_update_clk(dev);
        cmd->err = -RT_ETIMEOUT;
    }

    regval = readl(dev->base + REG_SMHC_CTRL);
    regval &= ~(1 << 4);
    writel(regval, dev->base + REG_SMHC_CTRL);
    regval = readl(dev->base + REG_SMHC_INTMASK);
    regval &= ~(0xffff);
    writel(regval, dev->base + REG_SMHC_INTMASK);
    writel(0xffffffff, dev->base + REG_SMHC_RINTSTS);
    regval = readl(dev->base + REG_SMHC_CTRL);
    regval |= (1 << 4);
    writel(regval, dev->base + REG_SMHC_CTRL);
    while (!rt_sem_take(&dev->smhc_sem, 0));
    mmcsd_req_complete(dev->host);

    return err;
}

static void t113_smhc_request(struct rt_mmcsd_host *host, struct rt_mmcsd_req *req)
{
    struct rt_mmcsd_data *data = RT_NULL;
    smhc_dev_t dev = (smhc_dev_t)host->private_data;

    dev->req = req;
    data = req->cmd->data;

    if (data)
    {
        dev->xfe.size = data->blksize;
        dev->xfe.num  = data->blks;
        dev->xfe.buff = (rt_uint8_t *)data->buf;
        dev->xfe.flag = (data->flags & DATA_DIR_WRITE) ? MMC_DATA_WRITE : MMC_DATA_READ;
    }

    rt_memset(&dev->flag, 0, sizeof(struct sdmmc_flag));
    sdmmc_send_cmd(host, req->cmd);
}

static void t113_smhc_set_iocfg(struct rt_mmcsd_host *host, struct rt_mmcsd_io_cfg *io_cfg)
{
    smhc_dev_t dev = (smhc_dev_t)host->private_data;
    rt_uint32_t clk = io_cfg->clock;
    rt_uint8_t width = io_cfg->bus_width;

    sdmmc_set_clk_width(dev, clk, width);
}

static rt_int32_t t113_smhc_get_card_status(struct rt_mmcsd_host *host)
{
    return RT_EOK;
}

static void enable_sdio_irq(struct rt_mmcsd_host *host, rt_int32_t en)
{
    LOG_D("enable_sdio_irq en %d", en);
}

static const struct rt_mmcsd_host_ops t113_smhc_ops =
{
    .request = t113_smhc_request,
    .set_iocfg = t113_smhc_set_iocfg,
    .get_card_status = t113_smhc_get_card_status,
    .enable_sdio_irq = enable_sdio_irq,
};

#ifdef BSP_USING_SMHC0
static struct smhc_dev smhc0 =
{
    .base = SMHC0_BASE_ADDR,
    .irq  = SMHC0_IRQ_NUM,
    .id   = 0,
    .smhc_clk = {.port=IO_PORTF,.pin=PIN_2,.mux=IO_PERIPH_MUX2,.pull=IO_PULL_UP},
    .smhc_cmd = {.port=IO_PORTF,.pin=PIN_3,.mux=IO_PERIPH_MUX2,.pull=IO_PULL_UP},
    .smhc_d0  = {.port=IO_PORTF,.pin=PIN_1,.mux=IO_PERIPH_MUX2,.pull=IO_PULL_UP},
    .smhc_d1  = {.port=IO_PORTF,.pin=PIN_0,.mux=IO_PERIPH_MUX2,.pull=IO_PULL_UP},
    .smhc_d2  = {.port=IO_PORTF,.pin=PIN_5,.mux=IO_PERIPH_MUX2,.pull=IO_PULL_UP},
    .smhc_d3  = {.port=IO_PORTF,.pin=PIN_4,.mux=IO_PERIPH_MUX2,.pull=IO_PULL_UP},
    .smhc_det = {.port=IO_PORTF,.pin=PIN_6,.mux=GPIO_INPUT    ,.pull=IO_PULL_NONE},
    .req      = RT_NULL,
    .host     = RT_NULL,
};
#endif /* BSP_USING_SMHC0 */

static void smhc_thread_entry(void *param)
{
    int current_status = 0;
    int old_status = 1;
    smhc_dev_t dev = (smhc_dev_t)param;

    while(1)
    {
        current_status = gpio_get_value(dev->smhc_det.port, dev->smhc_det.pin);
        if (current_status != old_status)
        {
            old_status = current_status;
            if (current_status == 1)
            {
                rt_kprintf("sdcard removed.\n");
            }
            else
            {
                rt_kprintf("sdcard inserted.\n");
            }
            mmcsd_change(dev->host);
        }
        rt_thread_mdelay(1000);
    }
}

int rt_hw_smhc_init(void)
{
    struct rt_mmcsd_host *host = RT_NULL;
    rt_thread_t tid = RT_NULL;

#ifdef BSP_USING_SMHC0
    host = mmcsd_alloc_host();
    if(host == 0)
    {
        LOG_E("alloc host failed");
        return -RT_ERROR;
    }

    if (rt_sem_init(&smhc0.smhc_sem, "smhc0_sem", RT_NULL, RT_IPC_FLAG_FIFO))
    {
        LOG_E("sem init failed");
        rt_free(host);
        return -RT_ERROR;
    }

    init_smhc0_clk(SMHC_SOURCE_CLK);

    iomux_set_sel(&smhc0.smhc_clk);
    iomux_set_sel(&smhc0.smhc_cmd);
    iomux_set_sel(&smhc0.smhc_d0);
    iomux_set_sel(&smhc0.smhc_d1);
    iomux_set_sel(&smhc0.smhc_d2);
    iomux_set_sel(&smhc0.smhc_d3);
    iomux_set_sel(&smhc0.smhc_det);

    rt_hw_interrupt_install(smhc0.irq, smhc_interrupt_handle, &smhc0, "smhc0_irq");
    rt_hw_interrupt_umask(smhc0.irq);

    writel((readl(smhc0.base + REG_SMHC_CTRL) | (0x01 << 4)), smhc0.base + REG_SMHC_CTRL);

    host->ops = &t113_smhc_ops;
    host->freq_min = 400 * 1000;
    host->freq_max = 50 * 1000 * 1000;
    host->valid_ocr = VDD_26_27 | VDD_27_28 | VDD_28_29 | VDD_29_30 | VDD_30_31 | VDD_31_32 |
    VDD_32_33 | VDD_33_34 | VDD_34_35 | VDD_35_36;
    host->flags = MMCSD_BUSWIDTH_4 | MMCSD_MUTBLKWRITE | MMCSD_SUP_SDIO_IRQ | MMCSD_SUP_HIGHSPEED;
    host->max_seg_size = 2048;
    host->max_blk_size = 512;
    host->max_blk_count = 4096;

    host->private_data = (void *)&smhc0;
    smhc0.host = host;

    tid = rt_thread_create("smhc", smhc_thread_entry, &smhc0, 4096, 20, 10);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
#endif /* BSP_USING_SMHC0 */

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_smhc_init);

static int sdmmc_reg_dump(void)
{
    LOG_I("---> [0x%08x] <---", smhc0.base);

    #define SDMMC_DUMP_REG(offset) \
        LOG_I("reg[0x%08x] = 0x%08x", smhc0.base + offset, readl(smhc0.base + offset));

    SDMMC_DUMP_REG(0x000);  // CTRL
    SDMMC_DUMP_REG(0x004);  // CLKDIV
    SDMMC_DUMP_REG(0x008);  // TMOUT
    SDMMC_DUMP_REG(0x00C);  // BUSWID
    SDMMC_DUMP_REG(0x010);  // BLKSIZ
    SDMMC_DUMP_REG(0x014);  // BYTCNT
    SDMMC_DUMP_REG(0x018);  // CMD
    SDMMC_DUMP_REG(0x01C);  // CMDARG
    SDMMC_DUMP_REG(0x020);  // RESP0
    SDMMC_DUMP_REG(0x024);  // RESP1
    SDMMC_DUMP_REG(0x028);  // RESP2
    SDMMC_DUMP_REG(0x02C);  // RESP3
    SDMMC_DUMP_REG(0x030);  // INTMASK
    SDMMC_DUMP_REG(0x034);  // MINTSTS
    SDMMC_DUMP_REG(0x038);  // RINTSTS
    SDMMC_DUMP_REG(0x03C);  // STATUS
    SDMMC_DUMP_REG(0x040);  // FIFOTH
    SDMMC_DUMP_REG(0x044);  // FUNS
    SDMMC_DUMP_REG(0x048);  // TCBCN
    SDMMC_DUMP_REG(0x04C);  // TBBCN
    SDMMC_DUMP_REG(0x050);  // DBGC
    SDMMC_DUMP_REG(0x054);  // CSDC
    SDMMC_DUMP_REG(0x058);  // A12A
    SDMMC_DUMP_REG(0x05C);  // NTSR
    SDMMC_DUMP_REG(0x060);  // SDBG
    SDMMC_DUMP_REG(0x078);  // HWRST
    SDMMC_DUMP_REG(0x080);  // DMAC
    SDMMC_DUMP_REG(0x084);  // DLBA
    SDMMC_DUMP_REG(0x088);  // DST
    SDMMC_DUMP_REG(0x08C);  // IDIE
    SDMMC_DUMP_REG(0x100);  // THLD
    SDMMC_DUMP_REG(0x104);  // SFC
    SDMMC_DUMP_REG(0x108);  // A23A
    SDMMC_DUMP_REG(0x10C);  // EMMC_DDR_SBIT_SET
    SDMMC_DUMP_REG(0x138);  // EXT_CMD
    SDMMC_DUMP_REG(0x13C);  // EXT_RESP
    SDMMC_DUMP_REG(0x140);  // DRV_DL
    SDMMC_DUMP_REG(0x144);  // SMAP_DL
    SDMMC_DUMP_REG(0x148);  // DS_DL
    SDMMC_DUMP_REG(0x14C);  // HS400_DL
    SDMMC_DUMP_REG(0x200);  // FIFO

    LOG_I("--------------------------------------------");

    return RT_EOK;
}
