#ifndef __DRV_NAND_H__
#define __DRV_NAND_H__

#include "board.h"
#include "drv_spi.h"

struct spi_nand_id
{
    unsigned char  mfr_id;
    unsigned short dev_id;
};

struct spi_nand_info
{
    struct spi_nand_id id;
    unsigned int page_size;
    unsigned int spare_size;
    unsigned int pages_per_block;
    unsigned int blocks_total;
};

struct spi_nand_handle
{
    struct spi_handle nand_spi;

    /* private */
    struct spi_nand_info info;
};

int nand_init(struct spi_nand_handle *nand);
int nand_deinit(struct spi_nand_handle *nand);
int nand_page_read(struct spi_nand_handle *nand, unsigned int page, unsigned char *data);
int nand_page_write(struct spi_nand_handle *nand, unsigned int page, unsigned char *data);
int nand_erase_page(struct spi_nand_handle *nand, unsigned int page);

#endif
