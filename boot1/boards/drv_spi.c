#include "drv_spi.h"
#include "drv_clk.h"
#include "interrupt.h"
#include "shell.h"

#define SPI_SORCE_CLK    200000000

enum spi_irq_state
{
    SPI_TC       = 0x1000,
    SPI_TF_UDF   = 0x0800,
    SPI_TF_OVF   = 0x0400,
    SPI_RX_UDF   = 0x0200,
    SPI_RX_OVF   = 0x0100,
    SPI_TX_FULL  = 0x0040,
    SPI_TX_EMP   = 0x0020,
    SPI_TX_RDY   = 0x0010,
    SPI_RX_FULL  = 0x0004,
    SPI_RX_EMP   = 0x0002,
    SPI_RX_RDY   = 0x0001,
};

enum spi_state
{
    SPI_ERROR   = 1,
    SPI_BUSY    = 2,
    SPI_IDLE    = 3,
};

static int spi_set_cs(unsigned int addr, enum spi_cs_select cs)
{
    unsigned int val = 0;

    if (cs > SPI_CS3 || cs < SPI_CS0)
    {
        return -1;
    }

    val = read32(addr + REG_SPI_TCR);
    val &= ~(0x3 << 4);
    val |= (cs << 4);
    write32(addr + REG_SPI_TCR, val);

    return 0;
}

static int spi_configure(unsigned int addr, struct spi_configure *cfg)
{
    unsigned int val = 0;

    if (cfg == U_NULL)
    {
        return -1;
    }

    val = read32(addr + REG_SPI_TCR);
    val &= ~(0x3 << 0); /* CPOL, CPHA = 0 */
    val &= ~((0x1 << 13) | (0x1 << 11)); /* SDC, SDM = 0 */
    val |= (0x1 << 8) | (0x1 << 7) | (0x1 << 2); /* SPI_TC_DHB | SPI_TC_SS_LEVEL | SPI_TC_SPOL */

    if (cfg->clk_rate >= 60000000)
    {
        val |= (0x1 << 11);
    }
    else if (cfg->clk_rate <= 24000000)
    {
        val |= (0x1 << 13);
    }

    /*1. POL */
    if (cfg->mode & SPI_MODE_CPOL)
    {
        val |= (0x1 << 1);
    }

    /*2. PHA */
    if (cfg->mode & SPI_MODE_CPHA)
    {
        val |= (0x1 << 0);
    }

    /*3. SSPOL,chip select signal polarity */
    if (cfg->mode & SPI_MODE_CS_HIGH)
    {
        val &= ~(0x1 << 2);
    }

    /*4. LMTF--LSB/MSB transfer first select */
    if (cfg->mode & SPI_MODE_LSB_FIRST)
    {
        val |= (0x1 << 12);
    }
    else
    {
        val &= ~(0x1 << 12);
    }

    /*master mode: set DDB,DHB,SMC,SSCTL*/
    /*5. dummy burst type */
    if (cfg->mode & SPI_MODE_DUMMY_ONE)
    {
        val |= (0x1 << 9);
    }
    else
    {
        val &= ~(0x1 << 9);
    }

    /*6.discard hash burst-DHB */
    if (cfg->mode & SPI_MODE_RECEIVE_ALL)
    {
        val &= ~(0x1 << 8);
    }
    else
    {
        val |= (0x1 << 8);
    }

    /*7. set SMC = 1 , SSCTL = 0 ,TPE = 1 */
    val &= ~(0x1 << 3);

    write32(addr + REG_SPI_TCR, val);

    return 0;
}

static void spi_start_xfer(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_TCR);
    val |= (1 << 31);
    write32(addr + REG_SPI_TCR, val);
}

static void spi_enable(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_GCR);
    val |= (0x1 << 0);
    write32(addr + REG_SPI_GCR, val);
}

static void spi_disable(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_GCR);
    val &= ~(0x1 << 0);
    write32(addr + REG_SPI_GCR, val);
}

static void spi_set_master_mode(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_GCR);
    val |= (0x1 << 1);
    write32(addr + REG_SPI_GCR, val);
}

/* enable transmit pause */
static void spi_enable_tx_pause(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_GCR);
    val |= (0x1 << 7);
    write32(addr + REG_SPI_GCR, val);
}

static void spi_soft_reset(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_GCR);
    val |= (0x1 << 31);
    write32(addr + REG_SPI_GCR, val);

    while (read32(addr + REG_SPI_GCR) & (1 << 31)); // Wait for reset bit to clear
}

static void spi_enable_irq(unsigned int addr, unsigned int irq_mask)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_IER);
    val |= (irq_mask & (0x77 | 0x3f << 8));
    write32(addr + REG_SPI_IER, val);
}

static void spi_disable_irq(unsigned int addr, unsigned int irq_mask)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_IER);
    val &= ~(irq_mask & (0x77 | 0x3f << 8));
    write32(addr + REG_SPI_IER, val);
}

static unsigned int spi_get_irq_state(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_ISR);
    val &= (0x77 | 0x3f << 8);

    return val;
}

static void spi_clear_irq_state(unsigned int addr, unsigned int irq_mask)
{
    unsigned int val = 0;

    val = irq_mask & (0x77 | 0x3f << 8);
    write32(addr + REG_SPI_ISR, val);
}

static unsigned int spi_get_txfifo_bytes(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_FSR) & (0xff << 16);

    val >>= 16;

    return val;
}

static unsigned int spi_get_rxfifo_bytes(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_FSR) & (0xff << 0);

    val >>= 0;

    return val;
}

static void spi_reset_fifo(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_FCR);
    val |= ((1 << 15) | (1 << 31));                     /* TX/RX FIFO Reset */
    val &= ~((0xff << 0) | (0xff << 16) | (0x1 << 8));  /* TX/RX FIFO trigger level clear */
    val |= (0x20 << 16) | (0x1 << 0);                   /* TX/RX FIFO trigger level set */
    write32(addr + REG_SPI_FCR, val);
    while (read32(addr + REG_SPI_FCR) & (1 << 31));     /* Wait for reset bit to clear */
}

/* set transfer total length BC, transfer length TC and single transmit length STC */
static void spi_set_bc_tc_stc(unsigned int addr, unsigned int tx_len, unsigned int rx_len,
    unsigned int stx_len, unsigned int dummy_cnt)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_MBC);
    val &= ~(0x00ffffff);
    val |= (0x00ffffff & (tx_len + rx_len + dummy_cnt));
    write32(addr + REG_SPI_MBC, val);

    val = read32(addr + REG_SPI_MTC);
    val &= ~(0x00ffffff);
    val |= (0x00ffffff & tx_len);
    write32(addr + REG_SPI_MTC, val);

    val = read32(addr + REG_SPI_BCC);
    val &= ~(0x00ffffff);
    val |= (0x00ffffff & stx_len);
    val &= ~(0xf << 24);
    val |= (dummy_cnt << 24);
    write32(addr + REG_SPI_BCC, val);
}

static void spi_ss_owner(unsigned int addr, unsigned int on_off)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_TCR);
    on_off &= 0x1;
    if (on_off)
    {
        val |= (0x1 << 6);
    }
    else
    {
        val &= ~(0x1 << 6);
    }
    write32(addr + REG_SPI_TCR, val);
}

static void spi_ss_level(unsigned int addr, unsigned int hi_lo)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_TCR);
    hi_lo &= 0x1;
    if (hi_lo)
    {
        val |= (0x1 << 7);
    }
    else
    {
        val &= ~(0x1 << 7);
    }
    write32(addr + REG_SPI_TCR, val);
}

static void spi_disable_dual(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_BCC);
    val &= ~(0x1 << 28);
    write32(addr + REG_SPI_BCC, val);
}

// static void spi_enable_dual(unsigned int addr)
// {
//     unsigned int val = 0;

//     val = read32(addr + REG_SPI_BCC);
//     val &= ~(0x1 << 29);
//     val |= (0x1 << 28);
//     write32(addr + REG_SPI_BCC, val);
// }

static void spi_disable_quad(unsigned int addr)
{
    unsigned int val = 0;

    val = read32(addr + REG_SPI_BCC);
    val &= ~(0x1 << 29);
    write32(addr + REG_SPI_BCC, val);
}

// static void spi_enable_quad(unsigned int addr)
// {
//     unsigned int val = 0;

//     val = read32(addr + REG_SPI_BCC);
//     val |= (0x1 << 29);
//     write32(addr + REG_SPI_BCC, val);
// }

static int spi_clk_init(unsigned int id, unsigned int mod_clk)
{
    unsigned int val  = 0;
    unsigned int addr = 0;
    unsigned int sclk = 0;
    unsigned int m, n, div, factor_m;

    /* Deassert spi0/spi1 reset */
    addr = 0x02001000 + 0x96c;
    val = read32(addr);
    val |= 1 << (16 + id);
    write32(addr, val);

    /* Open the spi0/spi1 bus gate */
    addr = 0x02001000 + 0x96c;
    val = read32(addr);
    val |= 1 << id;
    write32(addr, val);

    if (id == SPI0)
    {
        addr = 0x02001000 + 0x940;
    }
    else if (id == SPI1)
    {
        addr = 0x02001000 + 0x944;
    }
    else
    {
        return -1;
    }

    sclk = drv_clk_get_pll_peri_1x();
    div = (sclk + mod_clk - 1) / mod_clk;
    div = div == 0 ? 1 : div;
    if (div > 128)
    {
        m = 1;
        n = 0;
        return -1;
    }
    else if (div > 64)
    {
        n = 3;
        m = div >> 3;
    }
    else if (div > 32)
    {
        n = 2;
        m = div >> 2;
    }
    else if (div > 16)
    {
        n = 1;
        m = div >> 1;
    }
    else
    {
        n = 0;
        m = div;
    }

    factor_m = m - 1;

    /* CLK_SRC_SEL = PLL_PREI(1X) 600MHz */
    /* SCLK = src/M/N */
	/* N: 00:1 01:2 10:4 11:8 */
    /* M: factor_m + 1 */
    val = (1U << 31) | (0x1 << 24) | (n << 8) | factor_m;
    write32(addr, val);

    return 0;
}

static void spi_clk_deinit(unsigned int id)
{
    unsigned int val  = 0;
    unsigned int addr = 0;

    /* close the spi bus gate */
    addr = 0x02001000 + 0x96c;
    val = read32(addr);
    val &= ~(1 << id);
    write32(addr, val);

    /* Assert spi reset */
    addr = 0x02001000 + 0x96c;
    val = read32(addr);
    val &= ~(1 << (16 + id));
    write32(addr, val);
}

static void spi_set_clk(unsigned int addr, unsigned int spi_clk, unsigned int sclk_clk, unsigned int cdr)
{
    unsigned int val = 0;
    unsigned int div = 0;

    val = read32(addr + REG_SPI_CLK);
    if (cdr)
    {
        div = sclk_clk / (spi_clk * 2) - 1;
        val &= ~(0xff << 0);
        val |= (div | (0x1 << 12));
    }
    else
    {
        while(sclk_clk > spi_clk)
        {
            div++;
            sclk_clk >>= 1;
        }
        val &= ~((0xf << 8) | (0x1 << 12));
        val |= (div << 8);
    }
    write32(addr + REG_SPI_CLK, val);
}

static int spi_cpu_writeb(unsigned int addr, const unsigned char *buf)
{
    int poll_time = 0x7ffffff;

    while (poll_time > 0)
    {
        if (spi_get_txfifo_bytes(addr) > 63)
        {
            poll_time--;
        }
        else
        {
            write32(addr + REG_SPI_TXD, *(unsigned char *)buf);
            break;
        }
    }

    if (poll_time <= 0)
    {
        return -1;
    }

    return 0;
}

static int spi_cpu_readb(unsigned int addr, unsigned char *buf)
{
    int poll_time = 0x7ffffff;

    while (poll_time > 0)
    {
        if (spi_get_rxfifo_bytes(addr) > 0)
        {
            *(unsigned char *)buf = read8(addr + REG_SPI_RXD);
            break;
        }
        poll_time--;
    }

    if (poll_time <= 0)
    {
        return -1;
    }

    return 0;
}

static void spi_irq_handler(int irqno, void *param)
{
    unsigned int irq_state = 0;
    unsigned int byte_cnt  = 0;
    int i = 0;
    int ret = 0;
    struct spi_handle *spi = (struct spi_handle *)param;

    spi->spi_state = SPI_BUSY;

    irq_state = spi_get_irq_state(spi->base);

    s_printf("irq_state = 0x%04x\r\n", irq_state);

    if ((irq_state & (SPI_TF_UDF | SPI_TF_OVF | SPI_RX_UDF | SPI_RX_OVF)))
    {
        spi->spi_state = SPI_ERROR;
        return;
    }

    if (irq_state & SPI_RX_RDY)
    {
        byte_cnt = spi_get_rxfifo_bytes(spi->base);
        s_printf("byte_cnt = %d\r\n", byte_cnt);
        for (i = 0; i < byte_cnt; i++)
        {
            if (spi->rx_len > 0)
            {
                ret = spi_cpu_readb(spi->base, spi->rx_offset);
                if (ret != 0)
                {
                    spi->spi_state = SPI_ERROR;
                    return;
                }
                else
                {
                    spi->rx_len--;
                    spi->rx_offset++;
                }
            }
        }
    }

    if (irq_state & SPI_TC)
    {
        spi->spi_state = SPI_IDLE;
        return;
    }

    /* clear ISR */
    spi_clear_irq_state(spi->base, irq_state);
}

int spi_init(struct spi_handle *spi)
{
    int ret = 0;

    if (spi == U_NULL)
    {
        return -1;
    }

    if ((spi->base != SPI0_BASE_ADDR) && (spi->base!= SPI1_BASE_ADDR))
    {
        return -1;
    }

    /* config spi pins */
    ret += iomux_set_sel(&(spi->gpio_cs));
    ret += iomux_set_sel(&(spi->gpio_sck));
    ret += iomux_set_sel(&(spi->gpio_miso));
    ret += iomux_set_sel(&(spi->gpio_mosi));
    ret += iomux_set_sel(&(spi->gpio_wp));
    ret += iomux_set_sel(&(spi->gpio_hold));
    if (ret != 0)
    {
        return -1;
    }

    /* enable and configure spi source clock */
    ret = spi_clk_init(spi->id, SPI_SORCE_CLK);
    if (ret != 0)
    {
        return -1;
    }

    spi_set_clk(spi->base, spi->cfg.clk_rate, SPI_SORCE_CLK, 1);

    spi_soft_reset(spi->base);

    /* 1. enable the spi module */
    spi_enable(spi->base);

    /* 2. set the default chip select */
    ret = spi_set_cs(spi->base, spi->cfg.cs);
    if (ret != 0)
    {
        return -1;
    }

    /* 3. master: set spi module clock;
    */
    spi_set_master_mode(spi->base);

    /* 5. master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
    * 6. set bit width-default: 8 bits
    */
    spi_configure(spi->base, &spi->cfg);
    spi_ss_level(spi->base, 1);
    spi_enable_tx_pause(spi->base);

    /* 7. spi controller sends ss signal automatically*/
    spi_ss_owner(spi->base, 0);

    /* 8. reset fifo */
    spi_reset_fifo(spi->base);

    /* interrupt install */
    interrupt_install(spi->irq, spi_irq_handler, (void *)spi);
    interrupt_mask(spi->irq);

    return 0;
}

int spi_deinit(struct spi_handle *spi)
{
    if (spi == U_NULL)
    {
        return -1;
    }

    /* soft-reset the spi controller */
    spi_soft_reset(spi->base);

    /* disable the spi controller */
    spi_disable(spi->base);

    /* close the spi bus gate */
    spi_clk_deinit(spi->id);

    /* diable irq */
    interrupt_mask(spi->irq);
    spi_disable_irq(spi->base, 0xffff);

    return 0;
}

int spi_transfer(struct spi_handle *spi, struct spi_trans_msg *msg)
{
    unsigned int stx_len = 0;

    if (spi == U_NULL || msg == U_NULL)
    {
        return -1;
    }

    spi->tx_offset = msg->tx_buf;
    spi->tx_len    = msg->tx_len;
    spi->rx_offset = msg->rx_buf;
    spi->rx_len    = msg->rx_len;
    spi->dummylen  = msg->dummylen;

    /* Configure io mode */
    spi_disable_dual(spi->base);
    spi_disable_quad(spi->base);

    /* disable irq */
    interrupt_mask(spi->irq);
    spi_disable_irq(spi->base, 0xffff);

    stx_len = spi->tx_len + spi->rx_len;

    /* Configure SPI TX number and dummy counter */
    spi_set_bc_tc_stc(spi->base, spi->tx_len, spi->rx_len, stx_len, spi->dummylen);
    spi_reset_fifo(spi->base);
    spi_clear_irq_state(spi->base, 0xffff);
    spi_enable_irq(spi->base, (1 << 12)); /* TC Interrupt */
    spi_enable_irq(spi->base, (0xf << 8)); /* Error Interrupt */

    if (spi->rx_offset && spi->rx_len)
    {
        spi_enable_irq(spi->base, (1 << 0)); /* RX RDY Interrupt */
    }

    interrupt_umask(spi->irq);

    spi->spi_state = SPI_BUSY;

    spi_start_xfer(spi->base);

    while (spi->tx_len)
    {
        spi_cpu_writeb(spi->base, spi->tx_offset);
        spi->tx_len--;
        spi->tx_offset++;
    }

    while (spi->spi_state == SPI_BUSY);

    spi_disable_irq(spi->base, 0xffff);
    interrupt_mask(spi->irq);

    if (spi->spi_state == SPI_ERROR)
    {
        return -1;
    }

    return 0;
}
