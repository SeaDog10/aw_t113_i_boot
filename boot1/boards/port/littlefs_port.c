#include "littlefs_port.h"
#include "memheap.h"
#include "shell.h"

extern struct spi_nand_handle nand;

static unsigned char lfs_read_buffer[2048]      = {0};
static unsigned char lfs_write_buffer[2048]     = {0};
static unsigned char lfs_lookahead_buffer[2048] = {0};

static int lfs_read_port(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int lsf_prog_port(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int lfs_erase_port(const struct lfs_config *c, lfs_block_t block);
static int lfs_syncport(const struct lfs_config *c);

const struct lfs_config nand_lfs_cfg =
{
    .context            = &nand,
    .read               = lfs_read_port,
    .prog               = lsf_prog_port,
    .erase              = lfs_erase_port,
    .sync               = lfs_syncport,

    .read_size          = 2048,
    .prog_size          = 2048,
    .block_size         = 64 * 2048,
    .block_count        = 32,
    .cache_size         = 2048,
    .lookahead_size     = 2048,
    .block_cycles       = 500,

    .read_buffer        = lfs_read_buffer,
    .prog_buffer        = lfs_write_buffer,
    .lookahead_buffer   = lfs_lookahead_buffer,
};

static int lfs_read_port(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    int ret = 0;
    unsigned int page = 0;
    unsigned int offset = 0;

    page   = (block * 64) + off / nand.info.page_size;
    page  += PAGE_OFFSET_OF_NAND;
    offset = off % nand.info.page_size;

    // s_printf("lfs read page %d offset %d size %d\r\n", page, offset, size);

    ret = nand_page_read((struct spi_nand_handle *)c->context, (unsigned int)page,
        (unsigned int)offset, (unsigned char *)buffer, (unsigned int)size);

    if (ret != 0)
    {
        s_printf("lfs read page %d err\r\n", page);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

static int lsf_prog_port(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int ret = 0;
    unsigned int page = 0;
    unsigned int offset = 0;

    page   = (block * 64) + off / nand.info.page_size;
    page  += PAGE_OFFSET_OF_NAND;
    offset = off % nand.info.page_size;

    // s_printf("lfs write page %d offset %d size %d\r\n", page, offset, size);

    ret = nand_page_write((struct spi_nand_handle *)c->context, (unsigned int)page,
        (unsigned int)offset, (unsigned char *)buffer, (unsigned int)size);

    if (ret != 0)
    {
        s_printf("lfs write page %d err\r\n", page);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

static int lfs_erase_port(const struct lfs_config *c, lfs_block_t block)
{
    int ret = 0;
    unsigned int page = 0;

    page = block * 64;
    page += PAGE_OFFSET_OF_NAND;

    // s_printf("lfs erase page %d\r\n", page);

    ret = nand_erase_page((struct spi_nand_handle *)c->context, (unsigned int)page);

    if (ret!= 0)
    {
        s_printf("lfs erase page %d err.\r\n", page);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

static int lfs_syncport(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

lfs_t nand_lfs;

int drv_lfs_mount(void)
{
    int ret = 0;

    ret = lfs_mount(&nand_lfs, &nand_lfs_cfg);

    if (ret != LFS_ERR_OK)
    {
        s_printf("lfs_mount err. try format.\r\n");
        ret = lfs_format(&nand_lfs, &nand_lfs_cfg);
        if (ret!= LFS_ERR_OK)
        {
            s_printf("lfs_format err.\r\n");
            return ret;
        }

        ret = lfs_mount(&nand_lfs, &nand_lfs_cfg);
        if (ret!= LFS_ERR_OK)
        {
            return ret;
        }
    }

    s_printf("lfs_mount success.\r\n");

    return LFS_ERR_OK;
}
