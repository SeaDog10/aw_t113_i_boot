#include "partition_port.h"
#include "shell.h"

extern struct spi_nand_handle nand;

static unsigned char cache_erase[2048 * 64] = {0};

extern void dump_page(void *buffer, unsigned int len);

static int partition_nand_init(void)
{
    return 0;
}

static int partition_nand_read(unsigned int addr, unsigned char *buf, unsigned int size)
{
    int ret = 0;
    unsigned char *read_buf = U_NULL;
    unsigned int read_size = 0;
    unsigned int page = 0;
    unsigned int offset = 0;

    if (((addr + size) > (nand.info.page_size * nand.info.pages_per_block * nand.info.blocks_total)) || (buf == U_NULL))
    {
        return -1;
    }

    page   = addr / nand.info.page_size;
    offset = addr % nand.info.page_size;
    read_size = size;
    read_buf = buf;

    while (read_size != 0)
    {
        if ((offset + read_size) > nand.info.page_size)
        {
            ret = nand_page_read(&nand, page, offset, read_buf, nand.info.page_size - offset);
            if (ret != 0)
            {
                return ret;
            }
            read_size -= (nand.info.page_size - offset);
            read_buf += (nand.info.page_size - offset);
            page++;
            offset = 0;
        }
        else
        {
            ret = nand_page_read(&nand, page, offset, read_buf, read_size);
            if (ret != 0)
            {
                return ret;
            }
            read_size = 0;
        }
    }

    return ret;
}

static int partition_nand_write(unsigned int addr, unsigned char  *buf, unsigned int size)
{
    int ret = 0;
    unsigned char *write_buf = U_NULL;
    unsigned int write_size = 0;
    unsigned int page = 0;
    unsigned int offset = 0;

    if (((addr + size) > (nand.info.page_size * nand.info.pages_per_block * nand.info.blocks_total)) || (buf == U_NULL))
    {
        return -1;
    }

    page   = addr / nand.info.page_size;
    offset = addr % nand.info.page_size;
    write_size = size;
    write_buf = buf;

    while (write_size != 0)
    {
        if ((offset + write_size) > nand.info.page_size)
        {
            ret = nand_page_write(&nand, page, offset, write_buf, nand.info.page_size - offset);
            if (ret != 0)
            {
                return ret;
            }
            write_size -= (nand.info.page_size - offset);
            write_buf += (nand.info.page_size - offset);
            page++;
            offset = 0;
        }
        else
        {
            ret = nand_page_write(&nand, page, offset, write_buf, write_size);
            if (ret != 0)
            {
                return ret;
            }
            write_size = 0;
        }
    }

    return 0;
}

static int partition_nand_erase(unsigned int addr, unsigned int size)
{
    int ret = 0;
    int i   =0;
    unsigned char *buffer_ptr = U_NULL;
    unsigned int first_page_on_block = 0;
    unsigned int page = 0;
    unsigned int offset = 0;
    unsigned int erase_size = 0;

    if ((addr + size) > (nand.info.page_size * nand.info.pages_per_block * nand.info.blocks_total))
    {
        return -1;
    }

    page   = addr / nand.info.page_size;
    offset = addr % nand.info.page_size;
    erase_size = size;

    if ((offset + erase_size) > (nand.info.page_size * nand.info.pages_per_block))
    {
        return -1;
    }

    first_page_on_block = (page / 64) * 64;
    page -= first_page_on_block;

    buffer_ptr = cache_erase;
    for (i = 0; i < nand.info.pages_per_block; i++)
    {
        ret = nand_page_read(&nand, first_page_on_block + i, 0, buffer_ptr, nand.info.page_size);
        if (ret != 0)
        {
            return -1;
        }
        buffer_ptr += nand.info.page_size;
    }

    ret = nand_erase_page(&nand, first_page_on_block);
    if (ret != 0)
    {
        return -1;
    }

    i = page * 2048 + offset;

    while (erase_size != 0)
    {
        cache_erase[i] = 0xff;
        erase_size--;
        i++;
    }

    buffer_ptr = cache_erase;
    for (i = 0; i < nand.info.pages_per_block; i++)
    {
        ret = nand_page_write(&nand, first_page_on_block + i, 0, buffer_ptr, nand.info.page_size);
        if (ret != 0)
        {
            return -1;
        }
        buffer_ptr += nand.info.page_size;
    }

    return 0;
}

struct partition_dev_ops partition_nand_ops =
{
    .init = partition_nand_init,
    .read = partition_nand_read,
    .write = partition_nand_write,
    .erase = partition_nand_erase,
};

int partition_nand_register(void)
{
    char *dev_name = "nand_flash";
    int ret = 0;

    ret = partition_device_register(dev_name, &partition_nand_ops, nand.info.blocks_total * nand.info.pages_per_block * nand.info.page_size, nand.info.page_size);
    if (ret != 0)
    {
        return -1;
    }

    ret  = partition_register("boot0",       dev_name, 0 * 2048,    512 * 2048);
    ret += partition_register("boot1",       dev_name, 512 * 2048,  512 * 2048);
    ret += partition_register("Application", dev_name, 1024 * 2048, 512 * 2048);
    ret += partition_register("Download",    dev_name, 1536 * 2048, 512 * 2048);
    ret += partition_register("Factory",     dev_name, 2048 * 2048, 512 * 2048);
    ret += partition_register("LittleFs",    dev_name, 2560 * 2048, 2024 * 2048);

    if (ret != 0)
    {
        return -1;
    }

    show_partition_info();

    return 0;
}
