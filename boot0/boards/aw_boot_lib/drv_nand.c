#include "drv_nand.h"
#include "debug.h"
#include "memheap.h"

enum
{
    OPCODE_READ_ID           = 0x9f,
    OPCODE_READ_STATUS       = 0x0f,
    OPCODE_WRITE_STATUS      = 0x1f,
    OPCODE_READ_PAGE         = 0x13,
    OPCODE_READ              = 0x03,
    OPCODE_FAST_READ         = 0x0b,
    OPCODE_FAST_READ_DUAL_O  = 0x3b,
    OPCODE_FAST_READ_QUAD_O  = 0x6b,
    OPCODE_FAST_READ_DUAL_IO = 0xbb,
    OPCODE_FAST_READ_QUAD_IO = 0xeb,
    OPCODE_WRITE_ENABLE      = 0x06,
    OPCODE_BLOCK_ERASE       = 0xd8,
    OPCODE_PROGRAM_LOAD      = 0x02,
    OPCODE_PROGRAM_EXEC      = 0x10,
    OPCODE_RESET             = 0xff,
};

enum
{
    CONFIG_ADDR_PROTECT    = 0xa0,
    CONFIG_ADDR_OTP        = 0xb0,
    CONFIG_ADDR_STATUS     = 0xc0,
    CONFIG_POS_BUF         = 0x08, // Micron specific
};

struct nand_device nand_flash = {0};

static void *rt_memcpy(void *dst, const void *src, uint32_t count)
{
    char *tmp = (char *)dst, *s = (char *)src;
    uint32_t len;

    if (tmp <= s || tmp > (s + count))
    {
        while (count--)
            *tmp ++ = *s ++;
    }
    else
    {
        for (len = count; len > 0; len --)
            tmp[len - 1] = s[len - 1];
    }

    return dst;
}

static void *rt_memset(void *s, int c, uint32_t count)
{
    char *xs = (char *)s;

    while (count--)
        *xs++ = c;

    return s;
}

static uint8_t spi_nand_id(sunxi_spi_t *spi)
{
    uint8_t tx[1];
    uint8_t rx[4], *rxp;
    int32_t i, r;
    uint8_t mid = 0;
    // uint8_t did[2];

    tx[0] = OPCODE_READ_ID;
    r      = spi_transfer(spi, SPI_IO_SINGLE, tx, 1, rx, 4);
    if (r < 0)
    {
        return r;
    }

    if (rx[0] == 0xff)
    {
        rxp = rx + 1; // Dummy data, shift by one byte
    } else {
        rxp = rx;
    }

    mid    = rxp[0];
    // did[0] = rxp[1];
    // did[1] = rxp[2];

    return mid;
}

static int spi_nand_reset(sunxi_spi_t *spi)
{
    uint8_t tx[1];
    int        r;

    tx[0] = OPCODE_RESET;
    r     = spi_transfer(spi, SPI_IO_SINGLE, tx, 1, 0, 0);
    if (r < 0)
        return -1;

    udelay(100 * 1000);

    return 0;
}

static int spi_nand_get_config(sunxi_spi_t *spi, uint8_t addr, uint8_t *val)
{
    uint8_t tx[2];
    int        r;

    tx[0] = OPCODE_READ_STATUS;
    tx[1] = addr;
    r      = spi_transfer(spi, SPI_IO_SINGLE, tx, 2, val, 1);
    if (r < 0)
        return -1;

    return 0;
}

static int spi_nand_set_config(sunxi_spi_t *spi, uint8_t addr, uint8_t val)
{
    uint8_t tx[3];
    int        r;

    tx[0] = OPCODE_WRITE_STATUS;
    tx[1] = addr;
    tx[2] = val;
    r      = spi_transfer(spi, SPI_IO_SINGLE, tx, 3, 0, 0);
    if (r < 0)
        return -1;

    return 0;
}

static void spi_nand_wait_while_busy(sunxi_spi_t *spi)
{
    uint8_t tx[2];
    uint8_t rx[1];
    int        r;

    tx[0] = OPCODE_READ_STATUS;
    tx[1] = 0xc0; // SR3
    rx[0] = 0x00;

    do {
        r = spi_transfer(spi, SPI_IO_SINGLE, tx, 2, rx, 1);
        if (r < 0)
            break;
    } while ((rx[0] & 0x1) == 0x1); // SR3 Busy bit
}

int nand_init(struct nand_device *device, sunxi_spi_t *spi)
{
    int ret = 0;
    uint8_t val;

    if (!(device) || !(spi))
    {
        return -1;
    }

    device->spi = spi;

    spi_nand_reset(device->spi);
    spi_nand_wait_while_busy(device->spi);

    if (spi_nand_id(device->spi) != 0xcd)
    {
        error("SPI-NAND: ID not match\r\n");
        return -1;
    }

    spi_nand_get_config(spi, CONFIG_ADDR_OTP, &val);
    // debug("SPI-NAND: Configuration 0x%x\r\n", val);
    if(val & 0x01)
    {
        debug("SPI-NAND: disable F35SQA002G Quad mode\r\n");
        val &= ~(1 << 0);
        spi_nand_set_config(spi, CONFIG_ADDR_OTP, val);
        spi_nand_wait_while_busy(spi);
    }

    device->page_size       = 2048;
    device->oob_size        = 64;
    device->pages_per_block = 64;
    device->block_total     = 2048;

    device->page_buf = rt_memheap_alloc(&system_heap, device->page_size + device->oob_size + 10);
    device->temp_buf = rt_memheap_alloc(&system_heap, device->page_size + device->oob_size + 10);

    if (device->page_buf == 0 || device->temp_buf == 0)
    {
        error("SPI-NAND: memory alloc failed\r\n");
        return -1;
    }

    return 0;
}

static int spi_nand_load_page(sunxi_spi_t *spi, uint32_t page)
{
    uint32_t pa = page;
    uint8_t  tx[4];

    tx[0] = OPCODE_READ_PAGE;
    tx[1] = (uint8_t)(pa >> 16);
    tx[2] = (uint8_t)(pa >> 8);
    tx[3] = (uint8_t)(pa >> 0);

    spi_transfer(spi, SPI_IO_SINGLE, tx, 4, 0, 0);
    spi_nand_wait_while_busy(spi);

    return 0;
}

int nand_read_page(struct nand_device *device,uint32_t page,
                uint8_t *data, uint32_t data_len,
                uint8_t *spare, uint32_t spare_len)
{
    uint32_t len = 0;
    int32_t  read_opcode = OPCODE_READ;
    uint8_t  tx[4] = {0};
    uint8_t  val = 0;
    uint32_t ca = 0;

    if (page > (device->block_total * device->pages_per_block))
    {
        return -1;
    }

    len += device->page_size;
    len += device->oob_size;

    spi_nand_load_page(device->spi, page);

    tx[0] = read_opcode;
    tx[1] = 0;
    tx[2] = 0;
    tx[3] = 0x0;

    rt_memset(device->page_buf, 0x00, device->page_size + device->oob_size);

    spi_transfer(device->spi, SPI_IO_SINGLE, tx, 4, device->page_buf, len);
    spi_nand_wait_while_busy(device->spi);
    spi_nand_get_config(device->spi, CONFIG_ADDR_STATUS, &val);
    if (val & 0x20)
    {
        error("SPI-NAND: read ecc err\r\n");
        return -1;
    }

    if((data && (data_len > 0)) && (spare && (spare_len > 0)))
    {
        rt_memcpy(data, device->page_buf, data_len);
        rt_memcpy(spare, device->page_buf + device->page_size, spare_len);
    }
    else if(data && (data_len > 0))
    {
        rt_memcpy(data, device->page_buf, data_len);
    }
    else if(spare && (spare_len > 0))
    {
        rt_memcpy(spare, device->page_buf + device->page_size, spare_len);
    }

    return 0;
}

static int spi_nand_write_enable(sunxi_spi_t *spi)
{
    uint8_t     tx[2];

    tx[0] = OPCODE_WRITE_ENABLE;

    spi_transfer(spi, SPI_IO_SINGLE, tx, 1, 0, 0);
    spi_nand_wait_while_busy(spi);

    return 0;
}

static int spi_nand_exec_prog(sunxi_spi_t *spi, uint32_t page)
{
    uint32_t pa = page;
    uint8_t     tx[4];

    tx[0] = OPCODE_PROGRAM_EXEC;
    tx[1] = (uint8_t)(pa >> 16);
    tx[2] = (uint8_t)(pa >> 8);
    tx[3] = (uint8_t)(pa >> 0);

    spi_nand_write_enable(spi);
    spi_transfer(spi, SPI_IO_SINGLE, tx, 4, 0, 0);
    spi_nand_wait_while_busy(spi);

    return 0;
}

int nand_write_page(struct nand_device *device, uint32_t page,
                    const uint8_t *data, uint32_t data_len,
                    const uint8_t *spare, uint32_t spare_len)
{
    int ret = 0;
    uint32_t len = 0;
    int32_t  write_opcode = OPCODE_PROGRAM_LOAD;
    uint8_t  tx[3];
    uint8_t  val = 0;
    uint8_t *data_temp  = device->temp_buf;

    if (page > (device->block_total * device->pages_per_block))
    {
        return -1;
    }

    if(data && (data_len > 0))
    {
        len += device->page_size;
        if(spare && (spare_len > 0))
        {
            len += device->oob_size;
        }
    }
    else if(spare && (spare_len > 0))
    {
        len += device->page_size;
        len += device->oob_size;
    }
    else
    {
        return -1;
    }

    nand_read_page(device, page, data_temp, device->page_size, NULL, 0);

    rt_memset(device->page_buf, 0x00, device->page_size + device->oob_size);

    if((data && (data_len > 0)) && (spare && (spare_len > 0)))
    {
        rt_memcpy(device->page_buf, data, data_len);
        rt_memcpy(device->page_buf + device->page_size, spare, spare_len);
    }
    else if(data && (data_len > 0))
    {
        rt_memcpy(device->page_buf, data, data_len);
    }
    else if(spare && (spare_len > 0))
    {
        rt_memcpy(device->page_buf, data_temp, device->page_size);
        rt_memcpy(device->page_buf + device->page_size, spare, spare_len);
    }

    tx[0] = write_opcode;
    tx[1] = 0;
    tx[2] = 0;

    spi_nand_write_enable(device->spi);
    spi_transfer_then_transfer(device->spi, SPI_IO_SINGLE, tx, 3, device->page_buf, len);
    spi_nand_wait_while_busy(device->spi);
	spi_nand_get_config(device->spi, CONFIG_ADDR_STATUS, &val);
	if (val & 0x08)
	{
		error("SPI-NAND: program err\r\n");
		ret = -1;
	}

    spi_nand_exec_prog(device->spi, page);

    return ret;
}

int nand_move_page(struct nand_device *device, uint32_t src_page, uint32_t dst_page)
{
    int ret = 0;

    if ((src_page > (device->block_total * device->pages_per_block)) || (dst_page > (device->block_total * device->pages_per_block)))
    {
        return -1;
    }

    return ret;
}

int nand_erase_page(struct nand_device *device, uint32_t page)
{
    uint32_t pa = page;
    uint8_t  tx[4];
    uint8_t  val = 0;

    if (page > (device->block_total * device->pages_per_block))
    {
        return -1;
    }

    tx[0] = OPCODE_BLOCK_ERASE;
    tx[1] = (uint8_t)(pa >> 16);
    tx[2] = (uint8_t)(pa >> 8);
    tx[3] = (uint8_t)(pa >> 0);

	spi_nand_write_enable(device->spi);
    spi_transfer(device->spi, SPI_IO_SINGLE, tx, 4, 0, 0);
	spi_nand_wait_while_busy(device->spi);
    spi_nand_get_config(device->spi, CONFIG_ADDR_STATUS, &val);
	if (val & 0x04)
	{
		error("SPI-NAND: erase err\r\n");
		return -1;
	}

    return 0;
}

int nand_check_page(struct nand_device *device, uint32_t page)
{
    int32_t ret = 0;
    uint8_t buffer_temp[2];

    if (page > (device->block_total * device->pages_per_block))
    {
        return -1;
    }

    ret = nand_read_page(device, page, NULL, 0, buffer_temp, 2);
    if((buffer_temp[0] & buffer_temp[1]) != 0xFF)
    {
        ret = -1;
    }

    return ret;
}

int nand_mark_badpage(struct nand_device *device, uint32_t page)
{
    int32_t ret = 0;
    uint8_t buffer_temp[2];

    if (page > (device->block_total * device->pages_per_block))
    {
        return -1;
    }

    buffer_temp[0] = 0xBF;
    buffer_temp[1] = 0xFF;
    ret += nand_write_page(device, page, NULL, 0, buffer_temp, 2);

    return ret;
}
