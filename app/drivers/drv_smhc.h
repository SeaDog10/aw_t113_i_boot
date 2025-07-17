#ifndef __DRV_SMHC_H__
#define __DRV_SMHC_H__

#include <rtthread.h>

#define SMHC0_BASE_ADDR         (0x04020000)
#define SMHC1_BASE_ADDR         (0x04021000)
#define SMHC2_BASE_ADDR         (0x04022000)

#define SMHC0_IRQ_NUM           (72)
#define SMHC1_IRQ_NUM           (73)
#define SMHC2_IRQ_NUM           (74)

#define REG_SMHC_CTRL           (0x000)
#define REG_SMHC_CLKDIV         (0x004)
#define REG_SMHC_TMOUT          (0x008)
#define REG_SMHC_BUSWID         (0x00C)
#define REG_SMHC_BLKSIZ         (0x010)
#define REG_SMHC_BYTCNT         (0x014)
#define REG_SMHC_CMD            (0x018)
#define REG_SMHC_CMDARG         (0x01C)
#define REG_SMHC_RESP0          (0x020)
#define REG_SMHC_RESP1          (0x024)
#define REG_SMHC_RESP2          (0x028)
#define REG_SMHC_RESP3          (0x02C)
#define REG_SMHC_INTMASK        (0x030)
#define REG_SMHC_MINTSTS        (0x034)
#define REG_SMHC_RINTSTS        (0x038)
#define REG_SMHC_STATUS         (0x03C)
#define REG_SMHC_FIFOTH         (0x040)
#define REG_SMHC_FUNS           (0x044)
#define REG_SMHC_TCBCN          (0x048)
#define REG_SMHC_TBBCN          (0x04C)
#define REG_SMHC_DBGC           (0x050)
#define REG_SMHC_CSDC           (0x054)
#define REG_SMHC_A12A           (0x058)
#define REG_SMHC_NTSR           (0x05c)
#define REG_SMHC_SDBG           (0x060)
#define REG_SMHC_HWRST          (0x078)
#define REG_SMHC_DMAC           (0x080)
#define REG_SMHC_DLBA           (0x084)
#define REG_SMHC_DST            (0x088)
#define REG_SMHC_IDIE           (0x08c)
#define REG_SMHC_THLD           (0x100)
#define REG_SMHC_SFC            (0x104)
#define REG_SMHC_A23A           (0x108)
#define REG_EMMC_DDR_SBIT_SET   (0x10C)
#define REG_SMHC_EXT_CMD        (0x138)
#define REG_SMHC_EXT_RESP       (0x13C)
#define REG_SMHC_DRV_DL         (0x140)
#define REG_SMHC_SMAP_DL        (0x144)
#define REG_SMHC_DS_DL          (0x148)
#define REG_SMHC_HS400_DL       (0x14C)
#define REG_SMHC_FIFO           (0x200)

#define MMC_DATA_WRITE  (1 << 0)
#define MMC_DATA_READ   (1 << 1)

int rt_hw_smhc_init(void);

#endif /* __DRV_SMHC_H__ */
