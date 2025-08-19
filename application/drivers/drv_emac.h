#ifndef __DRV_EMAC_H__
#define __DRV_EMAC_H__

#include <rtthread.h>

#define EMAC_BASE_ADDR              (0x04500000)
#define EMAC_IRQ_NUM                (78)

#define REG_EMAC_BASIC_CTL0         (0x0000)
#define REG_EMAC_BASIC_CTL1         (0x0004)
#define REG_EMAC_INT_STA            (0x0008)
#define REG_EMAC_INT_EN             (0x000C)
#define REG_EMAC_TX_CTL0            (0x0010)
#define REG_EMAC_TX_CTL1            (0x0014)
#define REG_EMAC_TX_FLOW_CTL        (0x001C)
#define REG_EMAC_TX_DMA_DESC_LIST   (0x0020)
#define REG_EMAC_RX_CTL0            (0x0024)
#define REG_EMAC_RX_CTL1            (0x0028)
#define REG_EMAC_RX_DMA_DESC_LIST   (0x0034)
#define REG_EMAC_RX_FRM_FLT         (0x0038)
#define REG_EMAC_RX_HASH0           (0x0040)
#define REG_EMAC_RX_HASH1           (0x0044)
#define REG_EMAC_MII_CMD            (0x0048)
#define REG_EMAC_MII_DATA           (0x004C)
#define REG_EMAC_ADDR_HIGH0         (0x0050)
#define REG_EMAC_ADDR_LOW0          (0x0054)
#define REG_EMAC_ADDR_HIGHN(x)      (REG_EMAC_ADDR_HIGH0 + (x << 3))
#define REG_EMAC_ADDR_LOWN(x)       (REG_EMAC_ADDR_LOW0  + (x << 3))
#define REG_EMAC_TX_DMA_STA         (0x00B0)
#define REG_EMAC_TX_CUR_DESC        (0x00B4)
#define REG_EMAC_TX_CUR_BUF         (0x00B8)
#define REG_EMAC_RX_DMA_STA         (0x00C0)
#define REG_EMAC_RX_CUR_DESC        (0x00C4)
#define REG_EMAC_RX_CUR_BUF         (0x00C8)
#define REG_EMAC_RGMII_STA          (0x00D0)

#endif
