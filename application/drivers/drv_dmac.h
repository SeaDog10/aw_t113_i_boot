#ifndef __DRV_DMAC_H__
#define __DRV_DMAC_H__

#include <rtthread.h>

#define DMAC_BASE_ADDR              (0x03002000)
#define DMAC_IRQ_NUM                82

#define REG_DMAC_IRQ_EN0            0x00
#define REG_DMAC_IRQ_EN1            0x04
#define REG_DMAC_IRQ_PEND0          0x10
#define REG_DMAC_IRQ_PEND1          0x14
#define REG_DMAC_AUTO_GATE          0x28
#define REG_DMAC_STA                0x30

#define REG_DMAC_EN(x)              (0x0100 + (x * 0x0040))
#define REG_DMAC_PAU(x)             (0x0104 + (x * 0x0040))
#define REG_DMAC_DESC_ADDR(x)       (0x0108 + (x * 0x0040))
#define REG_DMAC_CFG(x)             (0x010c + (x * 0x0040))
#define REG_DMAC_CUR_SRC(x)         (0x0110 + (x * 0x0040))
#define REG_DMAC_CUR_DEST(x)        (0x0114 + (x * 0x0040))
#define REG_DMAC_BCNT_LEFT(x)       (0x0118 + (x * 0x0040))
#define REG_DMAC_PARA(x)            (0x011C + (x * 0x0040))
#define REG_DMAC_MODE(x)            (0x0128 + (x * 0x0040))
#define REG_DMAC_FDESC_ADDR(x)      (0x012c + (x * 0x0040))
#define REG_DMAC_PKG_NUM(x)         (0x0130 + (x * 0x0040))

#define T113_DMAC_CHANNEL_MAX   16

#define SUNXI_DMA_LINK_NULL	   (0xfffff800)

#define DMA_RST_OFS             16
#define DMA_GATING_OFS          0

#define DMAC_DMATYPE_NORMAL     0
#define DMAC_CFG_TYPE_DRAM      (1)
#define DMAC_CFG_TYPE_SRAM      (0)

#define DMA_PKG_HALF_INT_MASK    (1 << 0)
#define DMA_PKG_END_INT_MASK     (1 << 1)
#define DMA_QUEUE_END_INT_MASK   (1 << 2)

/* ----------DMA dest config-------------------- */
/* DMA dest width config */
#define DMAC_CFG_DEST_DATA_WIDTH_8BIT  (0x00)
#define DMAC_CFG_DEST_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_DEST_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_DEST_DATA_WIDTH_64BIT (0x03)

/* DMA dest bust config */
#define DMAC_CFG_DEST_1_BURST  (0x00)
#define DMAC_CFG_DEST_4_BURST  (0x01)
#define DMAC_CFG_DEST_8_BURST  (0x02)
#define DMAC_CFG_DEST_16_BURST (0x03)

#define DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE (0x00)
#define DMAC_CFG_DEST_ADDR_TYPE_IO_MODE     (0x01)

/* ----------DMA src config -------------------*/
#define DMAC_CFG_SRC_DATA_WIDTH_8BIT  (0x00)
#define DMAC_CFG_SRC_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_SRC_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_SRC_DATA_WIDTH_64BIT (0x03)

#define DMAC_CFG_SRC_1_BURST  (0x00)
#define DMAC_CFG_SRC_4_BURST  (0x01)
#define DMAC_CFG_SRC_8_BURST  (0x02)
#define DMAC_CFG_SRC_16_BURST (0x03)

#define DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE  (0x00)
#define DMAC_CFG_SRC_ADDR_TYPE_IO_MODE      (0x01)

/*dma int config*/
#define DMA_PKG_HALF_INT    (0)
#define DMA_PKG_END_INT     (1)
#define DMA_QUEUE_END_INT   (2)

typedef struct dma_channel_config
{
    volatile rt_uint32_t src_drq_type       : 6;
    volatile rt_uint32_t src_block_size     : 2;
    volatile rt_uint32_t src_addr_mode      : 1;
    volatile rt_uint32_t src_data_width     : 2;
    volatile rt_uint32_t reserved0          : 5;
    volatile rt_uint32_t dst_drq_type       : 6;
    volatile rt_uint32_t dst_block_size     : 2;
    volatile rt_uint32_t dst_addr_mode      : 1;
    volatile rt_uint32_t dst_data_width     : 2;
    volatile rt_uint32_t reserved1          : 5;
} dma_channel_config_t;

typedef struct
{
    dma_channel_config_t channel_cfg;
    rt_uint32_t loop_mode;
    rt_uint32_t data_block_size;
    rt_uint32_t wait_cyc;
    void (*dma_callback)(void);
    rt_uint32_t callback_type;
} dma_set_t;

void dma_init(void);
void dma_exit(void);

rt_uint32_t dma_request(rt_uint32_t dmatype);
rt_uint32_t dma_request_from_last(rt_uint32_t dmatype);

int dma_release(rt_uint32_t hdma);
int dma_setting(rt_uint32_t hdma, dma_set_t *cfg);
int dma_start(rt_uint32_t hdma, rt_uint32_t saddr, rt_uint32_t daddr, rt_uint32_t bytes);
int dma_stop(rt_uint32_t hdma);
int dma_querystatus(rt_uint32_t hdma);

#endif
