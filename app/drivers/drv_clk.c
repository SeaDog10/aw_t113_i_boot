#include <drv_clk.h>
#include <board.h>
#include <rtdbg.h>

#define DBG_TAG "[drv.clk]"
#define BSP_ENBALE_CLK_DEBUG 1

#define DBG_ENABLE
#if (BSP_ENBALE_CLK_DEBUG == 1)
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_WARNING
#endif
#define DBG_COLOR
#include <rtdbg.h>

static void sdelay(unsigned int loops)
{
    __asm__ volatile("1:\n"
                     "subs %0, %1, #1\n"
                     "bne 1b"
                     : "=r"(loops)
                     : "0"(loops));
}

/* 200MHz ~ 3GHz PLL_P default 0 */
static void init_pll_cpu(unsigned int freq)
{
    unsigned int val = 0;

    /* Step 1 Before you configure PLL_CPU, switch the clock source of CPU to PLL_PERI(1X) */
    writel((4 << 24) | (1 << 0), CCU_BASE_ADDR + REG_CCU_CPU_AXI_CFG);
    sdelay(10);

    /* Disable pll gating */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    val &= ~(1 << 27);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);

    /* Enable pll ldo */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    val |= (1 << 30);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    sdelay(5);

    /* Step 2 Modify the parameters N and P of PLL_CPU */
    /* PLL_CPUX = 24 MHz*N/P */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    val &= ~(0xff << 8);
    val |= (((freq / 24000000) - 1) << 8); /* PLL_N  PLL_P default 0 */
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);

    /* Step 3 Write the PLL Lock Enable bit (bit[29]) of PLL_CPU_CTRL_REG to 0 and then to 1 */
    /* Lock disable */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    val &= ~(1 << 29);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);

    /* Lock enable */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    val |= (1 << 29);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);

    /* Step 4 Wait for the Lock bit (bit[28]) of PLL_CPU_CTRL_REG to change to 1 */
    while (!(readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL) & (0x1 << 28)));
    sdelay(20);

    /* Enable pll gating */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    val |= (1 << 27);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);

    /* Lock disable */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    val &= ~(1 << 29);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    sdelay(1);

    /* Step 5 Switch the clock source of the CPU to PLL_CPU */
    val = readl(CCU_BASE_ADDR + REG_CCU_CPU_AXI_CFG);
    val &= ~(0x07 << 24 | 0x3 << 16 | 0x3 << 8 | 0xf << 0); /* Clear CPU_CLK_SEL PLL_P CPU_DEV1 CPU_DEV2 */
    val |= (0x03 << 24 | 0x0 << 16 | 0x1 << 8 | 0x1 << 0);  /* CPU_CLK_SEL=PLL_CPU/P PLL_P=0 CPU_DEV1=1 CPU_DEV2=1 */
    writel(val, CCU_BASE_ADDR + REG_CCU_CPU_AXI_CFG);
    sdelay(1);
}

/* The output clock of PLL_PERI(2X) is fixed to 1.2GHz and not suggested to change the parameter. */
static void init_pll_peri(void)
{
    unsigned int val = 0;

    if (readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL) & (1 << 31))
    {
        return;
    }

    /* Set default val */
    writel(0x63 << 8, CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);

    /* Lock enable */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);
    val |= (1 << 29);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);

    /* Enabe pll 600m(1x) 1200m(2x) */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);
    val |= (1 << 31);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);

    /* Wait pll stable */
    while (!(readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL) & (0x1 << 28)));
    sdelay(20);

    /* Lock disable */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);
    val &= ~(1 << 29);
    writel(val, CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);
}

static void init_ahb_clk(unsigned int freq)
{
    unsigned int val = 0;
    unsigned int m = 0;

    val = readl(CCU_BASE_ADDR + REG_CCU_PSI_CLK);
    val &= ~((0x3 << 24) | (0x3 << 8) | (0x3 << 0));

    /* PSI_CLK = Clock Source / M / N */
    val |= (0x3 << 24); /* Clock Source(PLL_PERI(1X) 600MHz) */
    val |= (0 << 8); /* N defalut 1 */
    m = drv_clk_get_pll_peri_1x() / freq;
    if ((m - 1) > 0x3)
    {
        m = 0x3;
    }
    val |= (m - 1) << 0; /* M */
    writel(val, CCU_BASE_ADDR + REG_CCU_PSI_CLK);
    sdelay(1);
}

static void init_apb0_clk(unsigned int freq)
{
    unsigned int val = 0;
    unsigned int m = 0;

    val = readl(CCU_BASE_ADDR + REG_CCU_APB0_CLK);
    val &= ~((0x3 << 24) | (0x3 << 8) | (0x3 << 0));

    /* PSI_CLK = Clock Source / M / N */
    val |= (0x3 << 24); /* Clock Source(PLL_PERI(1X) 600MHz) */
    val |= (0 << 8); /* N defalut 1 */
    m = drv_clk_get_pll_peri_1x() / freq;
    if ((m - 1) > 0xfe)
    {
        m = 0xfe;
    }
    val |= (m - 1) << 0; /* M */
    writel(val, CCU_BASE_ADDR + REG_CCU_APB0_CLK);
    sdelay(1);
}

static void init_apb1_clk(unsigned int freq)
{
    unsigned int val = 0;
    unsigned int m = 0;

    val = readl(CCU_BASE_ADDR + REG_CCU_APB1_CLK);
    val &= ~((0x3 << 24) | (0x3 << 8) | (0x3 << 0));

    /* PSI_CLK = Clock Source / M / N */
    val |= (0x3 << 24); /* Clock Source(PLL_PERI(1X) 600MHz) */
    val |= (0 << 8); /* N defalut 1 */
    m = drv_clk_get_pll_peri_1x() / freq;
    if ((m - 1) > 0xfe)
    {
        m = 0xfe;
    }
    val |= (m - 1) << 0; /* M */
    writel(val, CCU_BASE_ADDR + REG_CCU_APB1_CLK);
    sdelay(1);
}

static void init_usb0_clk(void)
{
    unsigned int val = 0;
    unsigned int bgr_addr = 0;
    unsigned int clk_addr = 0;

    clk_addr = (unsigned int)REG_CCU_USB0_CLK;
    bgr_addr = (unsigned int)REG_CCU_USB_BGR;

    /* otg gate open*/
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 8);
    writel(val, CCU_BASE_ADDR + bgr_addr);

    /* otg bus reset */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val &= ~(1 << 24);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 24);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);

    /* ehci gate open */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 4);
    writel(val, CCU_BASE_ADDR + bgr_addr);

    /* ehci bus reset */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val &= ~(1 << 20);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 20);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);

    /* ohci gate open */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 0);
    writel(val, CCU_BASE_ADDR + bgr_addr);

    /* ohci bus reset */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val &= ~(1 << 16);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 16);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);

    sdelay(10);

    /* clock enable */
    val = readl(CCU_BASE_ADDR + clk_addr);
    val &= ~(3 << 24);              /* Clear USB0_CLK_SEL */
    val |= (1 << 31) | (1 << 24);   /* Enable USB0_CLK enable & SEL 12M divided from 24 MHz */
    writel(val, CCU_BASE_ADDR + clk_addr);

    /* reset phy */
    val = readl(CCU_BASE_ADDR + clk_addr);
    val &= ~(1 << 30);
    writel(val, CCU_BASE_ADDR + clk_addr);
    sdelay(10);
    val = readl(CCU_BASE_ADDR + clk_addr);
    val |= (1 << 30);
    writel(val, CCU_BASE_ADDR + clk_addr);
    sdelay(10);
}

static void init_usb1_clk(void)
{
    unsigned int val = 0;
    unsigned int bgr_addr = 0;
    unsigned int clk_addr = 0;

    clk_addr = (unsigned int)REG_CCU_USB1_CLK;
    bgr_addr = (unsigned int)REG_CCU_USB_BGR;

    /* ehci gate open */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 5);
    writel(val, CCU_BASE_ADDR + bgr_addr);

    /* ehci bus reset */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val &= ~(1 << 21);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 21);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);

    /* ohci gate open */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 1);
    writel(val, CCU_BASE_ADDR + bgr_addr);

    /* ohci bus reset */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val &= ~(1 << 17);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 17);
    writel(val, CCU_BASE_ADDR + bgr_addr);
    sdelay(10);

    sdelay(10);

    /* clock enable */
    val = readl(CCU_BASE_ADDR + clk_addr);
    val &= ~(3 << 24);              /* Clear USB0_CLK_SEL */
    val |= (1 << 31) | (1 << 24);   /* Enable USB0_CLK enable & SEL 12M divided from 24 MHz */
    writel(val, CCU_BASE_ADDR + clk_addr);

    /* reset phy */
    val = readl(CCU_BASE_ADDR + clk_addr);
    val &= ~(1 << 30);
    writel(val, CCU_BASE_ADDR + clk_addr);
    sdelay(10);
    val = readl(CCU_BASE_ADDR + clk_addr);
    val |= (1 << 30);
    writel(val, CCU_BASE_ADDR + clk_addr);
    sdelay(10);
}

void init_smhc0_clk(unsigned int freq)
{
    unsigned int val = 0;
    unsigned int pll_clk = 0;
    unsigned int mod_clk = 0;
    unsigned int div = 0, n = 0;
    unsigned int bgr_addr = 0;
    unsigned int clk_addr = 0;

    clk_addr = REG_CCU_SMHC0_CLK;
    bgr_addr = REG_CCU_SMHC_BGR;

    /* reset SMHC0 */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= (1 << 16);
    writel(val, CCU_BASE_ADDR + bgr_addr);

    val = readl(CCU_BASE_ADDR + clk_addr);
    val &= ~(0x01 << 31);
    writel(val, CCU_BASE_ADDR + clk_addr);

    val &= ~(0x7 << 24); /* Clear clock source select */

    /* SMHC0_CLK = Clock Source/M/N. */
    if (freq == 0)
    {
        LOG_D("smhc0 freq set to 0.");
        return;
    }

    mod_clk = freq * 2;

    if (mod_clk <= 24000000)
    {
        /* Clock Source Select HOSC 24MHz  */
        val |= (0 << 24);
        pll_clk = 24000000;
    }
    else
    {
        /* Clock Source Select PLL_PERI(1X) 600MHz */
        val |= (0x01 << 24);
        pll_clk = drv_clk_get_pll_peri_1x();
    }

    div = pll_clk / mod_clk;
    if (pll_clk % mod_clk)
    {
        div++;
    }

    n = 0;
    while (div > 16)
    {
        n++;
        div = (div + 1) / 2;
    }

    if (n > 3)
    {
        LOG_E("smhc0 err.cannot set freq to %dHz", freq);
        return;
    }

    val |= (0x01 << 31); /* Gating clock ON */
    val |= (n << 8) | (div - 1);
    writel(val, CCU_BASE_ADDR + clk_addr);

    /* SMHC0 gate open */
    val = readl(CCU_BASE_ADDR + bgr_addr);
    val |= 0x01;
    writel(val, CCU_BASE_ADDR + bgr_addr);

    LOG_D("setfreq %dHz, s_clk:%dHz, M:%d, N:%d.", freq, pll_clk, div, n);
}

unsigned int drv_clk_get_pll_cpu(void)
{
    unsigned int val = 0;
    unsigned int p = 0;
    unsigned int n = 0;

    /* PLL_CPUX = 24 MHz*N/P */
    val = readl(CCU_BASE_ADDR + REG_CCU_CPU_AXI_CFG);
    val &= (0x3 << 16);
    p = 1 << val;

    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_CPU_CTRL);
    n = ((val >> 8) & 0xff) + 1;

    return (24000000 * n) / p;
}

unsigned int drv_clk_get_pll_peri_1x(void)
{
    unsigned int val = 0;
    unsigned int m = 0;
    unsigned int n = 0;
    unsigned int p = 0;

    /* PLL_PREI(1X) = 24MHz * N/M/P0/2 */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);
    p = ((val >> 16) & 0x7) + 1;
    n = ((val >> 8) & 0xff) + 1;
    m = (val & 0x1) + 1;

    return (24000000 * n) / (m * p * 2);
}

unsigned int drv_clk_get_pll_peri_2x(void)
{
    unsigned int val = 0;
    unsigned int m = 0;
    unsigned int n = 0;
    unsigned int p = 0;

    /* PLL_PREI(2X) = 24MHz * N/M/P0 */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);
    p = ((val >> 16) & 0x7) + 1;
    n = ((val >> 8) & 0xff) + 1;
    m = (val & 0x1) + 1;

    return (24000000 * n) / (m * p);
}

unsigned int drv_clk_get_pll_peri_800M(void)
{
    unsigned int val = 0;
    unsigned int m = 0;
    unsigned int n = 0;
    unsigned int p = 0;

    /* PLL_PREI(800M) = 24MHz * N/M/P1 */
    val = readl(CCU_BASE_ADDR + REG_CCU_PLL_PERI_CTRL);
    p = ((val >> 20) & 0x7) + 1;
    n = ((val >> 8) & 0xff) + 1;
    m = (val & 0x1) + 1;

    return (24000000 * n) / (m * p);
}

unsigned int drv_clk_get_ahbs_clk(void)
{
    unsigned int val = 0;
    unsigned int sclk = 0;
    unsigned int m = 0;
    unsigned int n = 0;

    /* PSI_CLK=CLOCK Source/M/N */
    val = readl(CCU_BASE_ADDR + REG_CCU_PSI_CLK);
    switch ((val >> 24) & 0x3)
    {
    case 0x0: /* HOSC */
        sclk = 24000000;
        break;
    case 0x1: /* CLK32K */
        sclk = 32000;
        break;
    case 0x2: /* CLK16M_RC */
        sclk = 16000000;
        break;
    case 0x3: /* PLL_PERI(1X) */
        sclk = drv_clk_get_pll_peri_1x();
        break;
    }

    m = ((val >> 0) & 0x3) + 1;
    n = 1 << ((val >> 8) & 0x3);

    return sclk / m / n;
}

unsigned int drv_clk_get_apb0_clk(void)
{
    unsigned int val = 0;
    unsigned int sclk = 0;
    unsigned int m = 0;
    unsigned int n = 0;

    /* APB0_CLK=CLOCK Source/M/N */
    val = readl(CCU_BASE_ADDR + REG_CCU_APB0_CLK);
    switch ((val >> 24) & 0x3)
    {
    case 0x0: /* HOSC */
        sclk = 24000000;
        break;
    case 0x1: /* CLK32K */
        sclk = 32000;
        break;
    case 0x2: /* PSI_CLK */
        sclk = drv_clk_get_ahbs_clk();
        break;
    case 0x3: /* PLL_PERI(1X) */
        sclk = drv_clk_get_pll_peri_1x();
        break;
    }

    m = ((val >> 0) & 0x1f) + 1;
    n = 1 << ((val >> 8) & 0x3);

    return sclk / m / n;
}

unsigned int drv_clk_get_apb1_clk(void)
{
    unsigned int val = 0;
    unsigned int sclk = 0;
    unsigned int m = 0;
    unsigned int n = 0;

    /* APB1_CLK=CLOCK Source/M/N */
    val = readl(CCU_BASE_ADDR + REG_CCU_APB1_CLK);
    switch ((val >> 24) & 0x3)
    {
    case 0x0: /* HOSC */
        sclk = 24000000;
        break;
    case 0x1: /* CLK32K */
        sclk = 32000;
        break;
    case 0x2: /* PSI_CLK */
        sclk = drv_clk_get_ahbs_clk();
        break;
    case 0x3: /* PLL_PERI(1X) */
        sclk = drv_clk_get_pll_peri_1x();
        break;
    }

    m = ((val >> 0) & 0x1f) + 1;
    n = 1 << ((val >> 8) & 0x3);

    return sclk / m / n;
}

int drv_clk_init(void)
{
    /* init PLL_CPU 1200MHz */
    init_pll_cpu(1200000000);

    /* init PLL_PERI(2X/1X) PLL_PERI(2X) 1200MHz PLL PLL_PERI(1X) 600MHz PLL_PERI(800M) 800MHz */
    init_pll_peri();

    /* init high-performance buses (AHBs) 200MHz */
    init_ahb_clk(200000000);

    /* init advanced peripheral buses (APB0) 200MHz */
    init_apb0_clk(200000000);

    /* init advanced peripheral buses (APB1) 24MHz */
    init_apb1_clk(24000000);

    /* init usb0 clk 12MHz */
    init_usb0_clk();

    /* init usb1 clk 12MHz */
    init_usb1_clk();

    /* init_smhc0_clk 24MHz */
    init_smhc0_clk(200000000);

    return 0;
}
INIT_BOARD_EXPORT(drv_clk_init);

static void dump_smhc0_clk(void)
{
    unsigned int val = 0;
    unsigned int bgr_addr = 0;
    unsigned int clk_addr = 0;

    clk_addr = REG_CCU_SMHC0_CLK;
    bgr_addr = REG_CCU_SMHC_BGR;

    val = readl(CCU_BASE_ADDR + clk_addr);
    rt_kprintf("SMHC CLK REG %p\n", val);

    val = readl(CCU_BASE_ADDR + bgr_addr);
    rt_kprintf("SMHC BGA REG %p\n", val);
}
MSH_CMD_EXPORT(dump_smhc0_clk, dump_smhc0_clk);
