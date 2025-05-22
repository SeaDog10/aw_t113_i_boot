#ifndef __DRV_NAND_H__
#define __DRV_NAND_H__

#include "sunxi_spi.h"
#include "board.h"

struct nand_device
{
    sunxi_spi_t *spi;
    uint8_t *page_buf;
    uint8_t *temp_buf;

    uint16_t page_size;          /* The Page size in the flash */
    uint16_t oob_size;           /* Out of bank size */

    uint32_t pages_per_block;    /* The number of page a block */
    uint16_t block_total;

    // /* Only be touched by driver */
    // uint32_t block_start;        /* The start of available block*/
    // uint32_t block_end;          /* The end of available block */
};

extern struct nand_device nand_flash;

int nand_read_page(struct nand_device *device, uint32_t page,
                uint8_t *data, uint32_t data_len,
                uint8_t *spare, uint32_t spare_len);
int nand_write_page(struct nand_device *device, uint32_t page,
                    const uint8_t *data, uint32_t data_len,
                    const uint8_t *spare, uint32_t spare_len);
int nand_move_page(struct nand_device *device, uint32_t src_page, uint32_t dst_page);
int nand_erase_page(struct nand_device *device, uint32_t page);

int nand_check_page(struct nand_device *device, uint32_t page);
int nand_mark_badpage(struct nand_device *device, uint32_t page);

int nand_init(struct nand_device *device, sunxi_spi_t *spi);

#endif
