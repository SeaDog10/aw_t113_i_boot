#include "drv_nand.h"

/* nand flash operation code */
enum
{
    OPCODE_SOFT_RESET               = 0xff,
    OPCODE_READ_ID                  = 0x9f,
    OPCODE_GET_FEATURE              = 0x0f,
    OPCODE_SET_FEATURE              = 0x1f,
    OPCODE_READ_PAGE                = 0x13,
    OPCODE_WRITE_ENABLE             = 0x06,
    OPCODE_WRITE_DISABLE            = 0x04,
    OPCODE_BLOCK_ERASE              = 0xd8,
    OPCODE_PROGRAM_LOAD             = 0x02,
    OPCODE_RANDOM_PROGRAM_LOAD      = 0x84,
    OPCODE_QUAD_PROGRAM_LOAD        = 0x32,
    OPCODE_RANDOM_QUAD_PROGRAM_LOAD = 0x34,
    OPCODE_PROGRAM_EXEC             = 0x10,
    OPCODE_PAGE_READ                = 0x13,
    OPCODE_READ_CACHE               = 0x03,
    OPCODE_READ_CACHE_X2            = 0x3b,
    OPCODE_READ_CACHE_X4            = 0x6b,
};

/* Status Registers addr */
enum
{
    SR_ADDR_PROTECT       = 0xa0,
    SR_ADDR_CONFIG        = 0xb0,
    SR_ADDR_STATUS        = 0xc0,
    SR_ADDR_S0_ECC_STATUS = 0x80,
    SR_ADDR_S1_ECC_STATUS = 0x84,
    SR_ADDR_S2_ECC_STATUS = 0x88,
    SR_ADDR_S3_ECC_STATUS = 0x8C,
};

static int nand_get_id(struct spi_nand_handle *nand)
{
    int ret = 0;
    unsigned char tx[1];
    unsigned char rx[4];
    struct spi_trans_msg msg;

    tx[0] = OPCODE_READ_ID;

    msg.tx_buf   = tx;
    msg.tx_len   = 1;
    msg.rx_buf   = rx;
    msg.rx_len   = 4;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    if (rx[0] == 0xff)
    {
        nand->info.id.mfr_id = rx[1];
        nand->info.id.dev_id = (rx[2] << 8) | (rx[3]);
    }
    else
    {
        return -1;
    }

    return 0;
}

static int nand_reset(struct spi_nand_handle *nand)
{
    int ret = 0;
    unsigned char tx[1];
    struct spi_trans_msg msg;

    tx[0] = OPCODE_SOFT_RESET;

    msg.tx_buf   = tx;
    msg.tx_len   = 1;
    msg.rx_buf   = U_NULL;
    msg.rx_len   = 0;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    us_delay(100 * 1000); /* wait for 100 ms */

    return 0;
}

static int nand_get_feature(struct spi_nand_handle *nand, unsigned char sr_addr, unsigned char *feature)
{
    int ret = 0;
    unsigned char tx[2];
    struct spi_trans_msg msg;

    tx[0] = OPCODE_GET_FEATURE;
    tx[1] = sr_addr;

    msg.tx_buf   = tx;
    msg.tx_len   = 2;
    msg.rx_buf   = feature;
    msg.rx_len   = 1;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

static int nand_set_feature(struct spi_nand_handle *nand, unsigned char sr_addr, unsigned char feature)
{
    int ret = 0;
    unsigned char tx[3];
    struct spi_trans_msg msg;

    tx[0] = OPCODE_SET_FEATURE;
    tx[1] = sr_addr;
    tx[2] = feature;

    msg.tx_buf   = tx;
    msg.tx_len   = 3;
    msg.rx_buf   = U_NULL;
    msg.rx_len   = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret!= 0)
    {
        return ret;
    }

    return 0;
}

static int nand_wait_while_busy(struct spi_nand_handle *nand)
{
    int ret = 0;
    unsigned char tx[2];
    unsigned char rx[1];
    struct spi_trans_msg msg;

    tx[0] = OPCODE_GET_FEATURE;
    tx[1] = SR_ADDR_STATUS;
    rx[0] = 0x00;

    msg.tx_buf   = tx;
    msg.tx_len   = 2;
    msg.rx_buf   = rx;
    msg.rx_len   = 1;
    msg.dummylen = 0;

    do
    {
        ret = spi_transfer(&nand->nand_spi, &msg);
        if (ret != 0)
        {
            return ret;
        }
        us_delay(100);
    } while ((rx[0] & 0x01) == 0x01);

    return 0;
}

static int nand_load_page(struct spi_nand_handle *nand, unsigned int page)
{
    int ret = 0;
    unsigned char tx[4];
    struct spi_trans_msg msg;

    tx[0] = OPCODE_PAGE_READ;
    tx[1] = (unsigned char)(page >> 16);
    tx[2] = (unsigned char)(page >> 8);
    tx[3] = (unsigned char)(page >> 0);

    msg.tx_buf   = tx;
    msg.tx_len   = 4;
    msg.rx_buf   = U_NULL;
    msg.rx_len   = 0;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_wait_while_busy(nand);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

static int nand_write_enable(struct spi_nand_handle *nand)
{
    int ret = 0;
    unsigned char tx[1];
    struct spi_trans_msg msg;

    tx[0] = OPCODE_WRITE_ENABLE;

    msg.tx_buf   = tx;
    msg.tx_len   = 1;
    msg.rx_buf   = U_NULL;
    msg.rx_len   = 0;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_wait_while_busy(nand);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

static int nand_exec_program(struct spi_nand_handle *nand, unsigned int page)
{
    int ret = 0;
    unsigned char tx[4];
    struct spi_trans_msg msg;

    ret = nand_write_enable(nand);
    if (ret != 0)
    {
        return ret;
    }

    tx[0] = OPCODE_PROGRAM_EXEC;
    tx[1] = (unsigned char)(page >> 16);
    tx[2] = (unsigned char)(page >> 8);
    tx[3] = (unsigned char)(page >> 0);

    msg.tx_buf   = tx;
    msg.tx_len   = 4;
    msg.rx_buf   = U_NULL;
    msg.rx_len   = 0;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_wait_while_busy(nand);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

int nand_init(struct spi_nand_handle *nand)
{
    int ret = 0;
    unsigned char val;

    if (nand == U_NULL)
    {
        return -1;
    }

    ret = spi_init(&nand->nand_spi);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_reset(nand);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_wait_while_busy(nand);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_get_id(nand);
    if (ret != 0)
    {
        return ret;
    }

    if ((nand->info.id.mfr_id != 0xcd) || (nand->info.id.dev_id != 0x7272))
    {
        return -1;
    }

    ret = nand_get_feature(nand, SR_ADDR_PROTECT, &val);
    if (ret != 0)
    {
        return ret;
    }
    val &= ~(0x1f << 2);
    val |= (1 << 2) | (0 << 6) | (1 << 5) | (0 << 4) | (1 << 3); /* protect 2M block 0~15 page 0~960 */
    ret = nand_set_feature(nand, SR_ADDR_PROTECT, val);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_get_feature(nand, SR_ADDR_CONFIG, &val);
    if (ret != 0)
    {
        return ret;
    }

    if (val & 0x01)
    {
        val &= ~0x01;
        ret = nand_set_feature(nand, SR_ADDR_CONFIG, val);
        if (ret != 0)
        {
            return ret;
        }

        ret = nand_wait_while_busy(nand);
        if (ret != 0)
        {
            return ret;
        }
    }

    nand->info.page_size       = 2048;
    nand->info.spare_size      = 64;
    nand->info.pages_per_block = 64;
    nand->info.blocks_total    = 2048;

    return 0;
}

int nand_deinit(struct spi_nand_handle *nand)
{
    int ret = 0;

    if (nand == U_NULL)
    {
        return -1;
    }

    ret = spi_deinit(&nand->nand_spi);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

int nand_page_read(struct spi_nand_handle *nand, unsigned int page, unsigned int offset,
    unsigned char *data, unsigned int len)
{
    int ret = 0;
    unsigned char tx[4];
    struct spi_trans_msg msg;
    unsigned char val = 0;

    if (nand == U_NULL || data == U_NULL)
    {
        return -1;
    }

    if (page > (nand->info.blocks_total * nand->info.pages_per_block))
    {
        return -1;
    }

    if ((offset > (nand->info.page_size)) || (len > (nand->info.page_size)))
    {
        return -1;
    }

    ret = nand_load_page(nand, page);
    if (ret != 0)
    {
        return ret;
    }

    tx[0] = OPCODE_READ_CACHE;
    tx[1] = (unsigned char)(offset >> 8);
    tx[2] = (unsigned char)(offset >> 0);
    tx[3] = 0x00;

    msg.tx_buf   = tx;
    msg.tx_len   = 4;
    msg.rx_buf   = data;
    msg.rx_len   = len;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_wait_while_busy(nand);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_get_feature(nand, SR_ADDR_STATUS, &val);
    if (ret != 0)
    {
        return ret;
    }

    // if (val & 0x20)
    // {
    //     return -1;
    // }

    return 0;
}

int nand_page_write(struct spi_nand_handle *nand, unsigned int page, unsigned int offset,
    unsigned char *data, unsigned int len)
{
    int ret = 0;
    unsigned char tx[3];
    unsigned char val = 0;
    struct spi_trans_msg first_msg;
    struct spi_trans_msg second_msg;

    if (nand == U_NULL || data == U_NULL)
    {
        return -1;
    }

    if (page > (nand->info.blocks_total * nand->info.pages_per_block))
    {
        return -1;
    }

    if ((offset > (nand->info.page_size)) || (len > (nand->info.page_size)))
    {
        return -1;
    }

    ret = nand_write_enable(nand);
    if (ret != 0)
    {
        return ret;
    }

    tx[0] = OPCODE_PROGRAM_LOAD;
    tx[1] = (unsigned char)(offset >> 8);
    tx[2] = (unsigned char)(offset >> 0);

    first_msg.tx_buf   = tx;
    first_msg.tx_len   = 3;
    first_msg.rx_buf   = U_NULL;
    first_msg.rx_len   = 0;
    first_msg.dummylen = 0;

    second_msg.tx_buf   = data;
    second_msg.tx_len   = len;
    second_msg.rx_buf   = U_NULL;
    second_msg.rx_len   = 0;
    second_msg.dummylen = 0;

    ret = spi_transfer_then_transfer(&nand->nand_spi, &first_msg, &second_msg);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_wait_while_busy(nand);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_get_feature(nand, SR_ADDR_STATUS, &val);
    if (ret != 0)
    {
        return ret;
    }

    if (val & 0x08)
    {
        return -1;
    }

    ret = nand_exec_program(nand, page);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

int nand_erase_page(struct spi_nand_handle *nand, unsigned int page)
{
    int ret = 0;
    unsigned char tx[4];
    struct spi_trans_msg msg;
    unsigned char val = 0;

    if (nand == U_NULL)
    {
        return -1;
    }

    if (page > (nand->info.blocks_total * nand->info.pages_per_block))
    {
        return -1;
    }

    ret = nand_write_enable(nand);
    if (ret != 0)
    {
        return ret;
    }

    tx[0] = OPCODE_BLOCK_ERASE;
    tx[1] = (unsigned char)(page >> 16);
    tx[2] = (unsigned char)(page >> 8);
    tx[3] = (unsigned char)(page >> 0);

    msg.tx_buf   = tx;
    msg.tx_len   = 4;
    msg.rx_buf   = U_NULL;
    msg.rx_len   = 0;
    msg.dummylen = 0;

    ret = spi_transfer(&nand->nand_spi, &msg);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_wait_while_busy(nand);
    if (ret != 0)
    {
        return ret;
    }

    ret = nand_get_feature(nand, SR_ADDR_STATUS, &val);
    if (ret != 0)
    {
        return ret;
    }

    if (val & 0x04)
    {
        return -1;
    }

    return 0;
}
