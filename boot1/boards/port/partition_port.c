#include "partition_port.h"

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
    unsigned int i = 0;
    unsigned int block_size = 0;
    unsigned int total_size = 0;
    unsigned int erase_start = 0;
    unsigned int erase_end = 0;
    unsigned int blk = 0;
    unsigned int block_start_index = 0;
    unsigned int block_end_index = 0;
    unsigned int blk_start_addr = 0;
    unsigned int blk_end_addr = 0;
    unsigned int offset = 0;
    unsigned int length = 0;
    unsigned int index = 0;
    unsigned char *buffer_ptr = U_NULL;

    block_size = nand.info.pages_per_block * nand.info.page_size;
    total_size = nand.info.blocks_total * block_size;

    if ((addr + size) > total_size)
    {
        return -1;
    }

    erase_start = addr;
    erase_end = addr + size;

    block_start_index = erase_start / block_size;
    block_end_index   = (erase_end - 1) / block_size;

    for (blk = block_start_index; blk <= block_end_index; blk++)
    {
        blk_start_addr = blk * block_size;
        blk_end_addr   = blk_start_addr + block_size;

        if (erase_start > blk_start_addr)
        {
            offset = erase_start - blk_start_addr;
        }
        else
        {
            offset = 0;
        }

        if (erase_end < blk_end_addr)
        {
            length = erase_end - (blk_start_addr + offset);
        }
        else
        {
            length = blk_end_addr - (blk_start_addr + offset);
        }

        if ((offset == 0) && (length == block_size))
        {
            ret = nand_erase_page(&nand, blk * nand.info.pages_per_block);
            if (ret != 0)
            {
                return -1;
            }
        }
        else
        {
            buffer_ptr = cache_erase;

            for (i = 0; i < nand.info.pages_per_block; i++)
            {
                ret = nand_page_read(&nand,
                                     blk * nand.info.pages_per_block + i,
                                     0,
                                     buffer_ptr,
                                     nand.info.page_size);
                if (ret != 0)
                {
                    return -1;
                }

                buffer_ptr += nand.info.page_size;
            }

            ret = nand_erase_page(&nand, blk * nand.info.pages_per_block);
            if (ret != 0)
            {
                return -1;
            }

            for (index = 0; index < length; index++)
            {
                cache_erase[offset + index] = 0xFF;
            }

            buffer_ptr = cache_erase;

            for (i = 0; i < nand.info.pages_per_block; i++)
            {
                ret = nand_page_write(&nand,
                                      blk * nand.info.pages_per_block + i,
                                      0,
                                      buffer_ptr,
                                      nand.info.page_size);
                if (ret != 0)
                {
                    return -1;
                }

                buffer_ptr += nand.info.page_size;
            }
        }
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

static int partiton_func(int argc, char **argv)
{
    int ret = 0;
    char *option;
    char *part_name;
    char buf[16];
    int i = 0;
    int offset;
    int len;

    if (argc < 2)
    {
        show_partition_info();
        return 0;
    }

    option = argv[1];

    if (xstrncmp(option, "help", sizeof("help")) == 0)
    {
        s_printf("Usage: \r\n");
        s_printf(" show help info     - help \r\n");
        s_printf(" read partiton data - read <partition name> <offset> <len>\r\n");
    }

    if (xstrncmp(option, "read", sizeof("read")) == 0)
    {
        if (argc < 5)
        {
            s_printf("Usage : \r\n");
            s_printf("partition read <partition name> <offset> <len>\r\n");
        }
        else
        {
            part_name = argv[2];

            ret = xstrtol(argv[3], &offset);
            if (ret != 0)
            {
                s_printf("Invalid offset: %s\r\n", argv[3]);
            }

            ret = xstrtol(argv[4], &len);
            if (ret != 0)
            {
                s_printf("Invalid len: %s\r\n", argv[4]);
            }

            while (len)
            {
                if (len >= sizeof(buf))
                {
                    ret = partition_read(part_name, buf, offset, sizeof(buf));
                    if (ret != 0)
                    {
                        s_printf("read patition %s offset %d err.\r\n", part_name, offset);
                        return -1;
                    }
                    offset += sizeof(buf);
                    len    -= sizeof(buf);
                    for (i = 0; i < sizeof(buf); i++)
                    {
                        s_printf("%02x ", buf[i]);
                    }
                    s_printf("\r\n");
                }
                else
                {
                    ret = partition_read(part_name, buf, offset, len);
                    if (ret != 0)
                    {
                        s_printf("read patition %s offset %d err.\r\n", part_name, offset);
                        return -1;
                    }
                    len = 0;
                    for (i = 0; i < len; i++)
                    {
                        s_printf("%02x ", buf[i]);
                    }
                    s_printf("\r\n");
                }
            }
        }
    }

    return 0;
}

static struct shell_command partition_cmd =
{
    .name = "partition",
    .desc = "partiton options",
    .func = partiton_func,
    .next = SHELL_NULL,
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

    ret  = partition_register("boot0",       dev_name, 0 * 2048,    512 * 2048); /* 2M first boot           */
    ret += partition_register("boot1",       dev_name, 512 * 2048,  512 * 2048); /* 2M second boot/ota      */
    ret += partition_register("APP1",        dev_name, 1024 * 2048, 512 * 2048); /* 2M application img 1    */
    ret += partition_register("APP2",        dev_name, 1536 * 2048, 512 * 2048); /* 2M application img 2    */
    ret += partition_register("Download",    dev_name, 2048 * 2048, 512 * 2048); /* 2M application download */
    ret += partition_register("Param",       dev_name, 2560 * 2048,  60 * 2048); /* ota parameters */
    // ret += partition_register("LittleFs",    dev_name, 2560 * 2048, 2024 * 2048);

    if (ret != 0)
    {
        return -1;
    }

    show_partition_info();

    shell_register_command(&partition_cmd);

    return 0;
}
