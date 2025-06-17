#ifndef __DRV_SPI_H__
#define __DRV_SPI_H__

#include "board.h"
#include "drv_iomux.h"

#define SPI0_BASE_ADDR          0x04025000
#define SPI1_BASE_ADDR          0x04026000

#define SPI0_IRQ                47
#define SPI1_IRQ                48

#define REG_SPI_GCR             0x0004
#define REG_SPI_TCR             0x0008
#define REG_SPI_IER             0x0010
#define REG_SPI_ISR             0x0014
#define REG_SPI_FCR             0x0018
#define REG_SPI_FSR             0x001c
#define REG_SPI_WCR             0x0020
#define REG_SPI_CLK             0x0024
#define REG_SPI_SAMP_DL         0x0028
#define REG_SPI_MBC             0x0030
#define REG_SPI_MTC             0x0034
#define REG_SPI_BCC             0x0038
#define REG_SPI_BATCR           0x0040
#define REG_SPI_BA_CCR          0x0044
#define REG_SPI_TBR             0x0048
#define REG_SPI_RBR             0x004c
#define REG_SPI_NDMA_MODE_CTL   0x0088
#define REG_SPI_TXD             0x0200
#define REG_SPI_RXD             0x0300

#define SPI_MODE_CPOL           (0x01)
#define SPI_MODE_CPHA           (0x02)
#define SPI_MODE_CS_HIGH        (0x04)
#define SPI_MODE_LSB_FIRST      (0x08)
#define SPI_MODE_DUMMY_ONE      (0x10)
#define SPI_MODE_RECEIVE_ALL    (0x20)

enum spi_id
{
    SPI0 = 0,
    SPI1 = 1,
};

enum spi_cs_select
{
    SPI_CS0   = 0,
    SPI_CS1   = 1,
    SPI_CS2   = 2,
    SPI_CS3   = 3,
};

struct spi_configure
{
    unsigned int       clk_rate;
    unsigned int       mode;
    enum spi_cs_select cs;
};

struct spi_trans_msg
{
    unsigned char *tx_buf;
    unsigned int  tx_len;
    unsigned char *rx_buf;
    unsigned int  rx_len;
    unsigned int  dummylen;
};

struct spi_handle
{
    /* user define */
    unsigned int          base;
    unsigned int          irq;
    enum spi_id           id;
    struct spi_configure  cfg;
    struct iomux_cfg gpio_cs;
    struct iomux_cfg gpio_sck;
    struct iomux_cfg gpio_miso;
    struct iomux_cfg gpio_mosi;
    struct iomux_cfg gpio_wp;
    struct iomux_cfg gpio_hold;
    /* private */
    unsigned char *tx_offset;
    unsigned int  tx_len;
    unsigned char *rx_offset;
    unsigned int  rx_len;
    unsigned int  dummylen;
    unsigned int  spi_state;
};

int spi_init(struct spi_handle *spi);
int spi_deinit(struct spi_handle *spi);
int spi_transfer(struct spi_handle *spi, struct spi_trans_msg *msg);

/* only send no recive */
int spi_transfer_then_transfer(struct spi_handle *spi, struct spi_trans_msg *msg_first, struct spi_trans_msg *msg_second);
#endif
