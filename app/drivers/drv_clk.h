#ifndef __DRV_CLK_H__
#define __DRV_CLK_H__

#include <rtthread.h>

#define CCU_BASE_ADDR                   0x02001000

#define REG_CCU_PLL_CPU_CTRL            0x000
#define REG_CCU_PLL_DDR_CTRL            0x010
#define REG_CCU_PLL_PERI_CTRL           0x020
#define REG_CCU_PLL_VIDEO0_CTRL         0x040
#define REG_CCU_PLL_VIDEO1_CTRL         0x048
#define REG_CCU_PLL_VE_CTRL             0x058
#define REG_CCU_PLL_AUDIO0_CTRL         0x078
#define REG_CCU_PLL_AUDIO1_CTRL         0x080
#define REG_CCU_PLL_DDR_PAT0_CTRL       0x110
#define REG_CCU_PLL_DDR_PAT1_CTRL       0x114
#define REG_CCU_PLL_PERI0_PAT0_CTRL     0x120
#define REG_CCU_PLL_PERI0_PAT1_CTRL     0x124
#define REG_CCU_PLL_VIDEO0_PAT0_CTRL    0x140
#define REG_CCU_PLL_VIDEO0_PAT1_CTRL    0x144
#define REG_CCU_PLL_VIDEO1_PAT0_CTRL    0x148
#define REG_CCU_PLL_VIDEO1_PAT1_CTRL    0x14c
#define REG_CCU_PLL_VE_PAT0_CTRL        0x158
#define REG_CCU_PLL_VE_PAT1_CTRL        0x15c
#define REG_CCU_PLL_AUDIO0_PAT0_CTRL    0x178
#define REG_CCU_PLL_AUDIO0_PAT1_CTRL    0x17c
#define REG_CCU_PLL_AUDIO1_PAT0_CTRL    0x180
#define REG_CCU_PLL_AUDIO1_PAT1_CTRL    0x184
#define REG_CCU_PLL_CPU_BIAS            0x300
#define REG_CCU_PLL_DDR_BIAS            0x310
#define REG_CCU_PLL_PERI0_BIAS          0x320
#define REG_CCU_PLL_VIDEO0_BIAS         0x340
#define REG_CCU_PLL_VIDEO1_BIAS         0x348
#define REG_CCU_PLL_VE_BIAS             0x358
#define REG_CCU_PLL_AUDIO0_BIAS         0x378
#define REG_CCU_PLL_AUDIO1_BIAS         0x380
#define REG_CCU_PLL_CPU_TUN             0x400
#define REG_CCU_CPU_AXI_CFG             0x500
#define REG_CCU_CPU_GATING              0x504
#define REG_CCU_PSI_CLK                 0x510
#define REG_CCU_APB0_CLK                0x520
#define REG_CCU_APB1_CLK                0x524
#define REG_CCU_MBUS_CLK                0x540
#define REG_CCU_DMA_BGR                 0x70c
#define REG_CCU_DRAM_CLK                0x800
#define REG_CCU_MBUS_MAT_CLK_GATING     0x804
#define REG_CCU_DRAM_BGR                0x80c
#define REG_CCU_SMHC0_CLK               0x830
#define REG_CCU_SMHC1_CLK               0x834
#define REG_CCU_SMHC2_CLK               0x838
#define REG_CCU_SMHC_BGR                0x84c
#define REG_CCU_USART_BGR               0x90c
#define REG_CCU_TWI_BGR                 0x91c
#define REG_CCU_SPI0_CLK                0x940
#define REG_CCU_SPI_BGR                 0x96c
#define REG_CCU_EMAC_CLK                0x970
#define REG_CCU_EMAC_BGR                0x97c
#define REG_CCU_USB0_CLK                0xA70
#define REG_CCU_USB1_CLK                0xA74
#define REG_CCU_USB_BGR                 0xA8C
#define REG_CCU_RISCV_CLK               0xd00
#define REG_CCU_RISCV_GATING            0xd04
#define REG_CCU_RISCV_CFG_BGR           0xd0c

/* MMC clock bit field */
#define CCU_MMC_CTRL_M(x)               ((x)-1)
#define CCU_MMC_CTRL_N(x)               ((x) << 8)
#define CCU_MMC_CTRL_OSCM24             (0x0 << 24)
#define CCU_MMC_CTRL_PLL6X1             (0x1 << 24)
#define CCU_MMC_CTRL_PLL6X2             (0x2 << 24)
#define CCU_MMC_CTRL_PLL_PERIPH1X       CCU_MMC_CTRL_PLL6X1
#define CCU_MMC_CTRL_PLL_PERIPH2X       CCU_MMC_CTRL_PLL6X2
#define CCU_MMC_CTRL_ENABLE             (0x1 << 31)
/* if doesn't have these delays */
#define CCU_MMC_CTRL_OCLK_DLY(a) ((void)(a), 0)
#define CCU_MMC_CTRL_SCLK_DLY(a) ((void)(a), 0)

#define CCU_MMC_BGR_SMHC0_GATE (1 << 0)
#define CCU_MMC_BGR_SMHC1_GATE (1 << 1)
#define CCU_MMC_BGR_SMHC2_GATE (1 << 2)

#define CCU_MMC_BGR_SMHC0_RST (1 << 16)
#define CCU_MMC_BGR_SMHC1_RST (1 << 17)
#define CCU_MMC_BGR_SMHC2_RST (1 << 18)

int drv_clk_init(void);

unsigned int drv_clk_get_pll_cpu(void);
unsigned int drv_clk_get_pll_peri_1x(void);
unsigned int drv_clk_get_pll_peri_2x(void);
unsigned int drv_clk_get_pll_peri_800M(void);

unsigned int drv_clk_get_ahbs_clk(void);
unsigned int drv_clk_get_apb0_clk(void);
unsigned int drv_clk_get_apb1_clk(void);

void init_smhc0_clk(unsigned int freq);

#endif
