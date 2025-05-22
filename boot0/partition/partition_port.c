#include "partition.h"

#define FLASH_ADDR 0x40020000
#define FLASH_SIZE 0x02000000

int ram_init(void)
{
    return 0;
}

int ram_read(unsigned int addr, unsigned char *buf, unsigned int size)
{
    if(buf == NULL || (addr + size) > FLASH_SIZE)
    {
        return -1;
    }

    memcpy(buf, (void*)(addr), size);

    return 0;
}

int ram_write(unsigned int addr, const unsigned char  *buf, unsigned int size)
{
    if(buf == NULL || (addr + size) > FLASH_SIZE)
    {
        return -1;
    }

    memcpy((void*)(addr), buf, size);

    return 0;
}

int ram_erase(unsigned int addr, unsigned int size)
{
    if((addr + size) > FLASH_SIZE)
    {
        return -1;
    }

    memset((void*)(addr), 1, size);

    return 0;
}

struct partition_device ram_dev = {
    .size = FLASH_SIZE,
    .block_size = 4096,
    .erase_before_write = 1,
    .ops.init  = ram_init,
    .ops.read  = ram_read,
    .ops.write = ram_write,
    .ops.erase = ram_erase,
};

struct partition tables[] =
{
    {
        .start = FLASH_ADDR,
        .size = 0x00100000,
    },
    {
        .start = FLASH_ADDR + 0x00100000,
        .size = 0x00100000,
    },
    {
        .start = FLASH_ADDR + 0x00200000,
        .size = 0x00100000,
    },
    {
        .start = FLASH_ADDR + 0x00300000,
        .size = 0x00100000,
    },
    {
        .start = FLASH_ADDR + 0x00400000,
        .size = 0x00100000,
    },
};

#include "debug.h"

void partition_test(void)
{
    int ret = 0;

    ret = partition_device_register(&ram_dev, "ramdev1");
    if(ret != 0)
    {
        debug("partition_device_register failed!\r\n");
        return;
    }
    debug("partition_device_register success!\r\n");

    ret = partition_device_init(&ram_dev);
    if(ret!= 0)
    {
        debug("partition_device_init failed!\r\n");
        return;
    }
    debug("partition_device_init success!\r\n");

    ret = partition_register(&tables[0], "ramdev1", "boot");
    if(ret!= 0)
    {
        debug("partition1 register failed!\r\n");
        return;
    }
    debug("partition1 register success!\r\n");

    ret = partition_register(&tables[1], "ramdev1", "backup");
    if(ret!= 0)
    {
        debug("partition2 register failed!\r\n");
        return;
    }
    debug("partition2 register success!\r\n");

    ret = partition_register(&tables[2], "ramdev1", "app");
    if(ret!= 0)
    {
        debug("partition3 register failed!\r\n");
        return;
    }
    debug("partition3 register success!\r\n");

    ret = partition_register(&tables[3], "ramdev1", "download");
    if(ret!= 0)
    {
        debug("partition4 register failed!\r\n");
        return;
    }
    debug("partition4 register success!\r\n");

    ret = partition_register(&tables[4], "ramdev1", "reserve");
    if(ret!= 0)
    {
        debug("partition5 register failed!\r\n");
        return;
    }
    debug("partition5 register success!\r\n");

    show_partition_info();
}

