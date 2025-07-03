#include "partition/partition.h"

#define PARTITION_NULL            0

#if PARTITION_LOG_LEVEL > PARTITION_LOG_NONE
#include "shell/shell.h"
#endif

#if PARTITION_LOG_LEVEL >= PARTITION_LOG_TRACE
#define PART_TRACE(format, ...) do{s_printf("[T]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define PART_TRACE(format, ...)
#endif

#if PARTITION_LOG_LEVEL >= PARTITION_LOG_DEBUG
#define PART_DEBUG(format, ...) do{s_printf("[D]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define PART_DEBUG(format, ...)
#endif

#if PARTITION_LOG_LEVEL >= PARTITION_LOG_INFO
#define PART_INFO(format, ...) do{s_printf("[I]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define PART_INFO(format, ...)
#endif

#if PARTITION_LOG_LEVEL >= PARTITION_LOG_WARN
#define PART_WARN(format, ...) do{s_printf("[W]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define PART_WARN(format, ...)
#endif

#if PARTITION_LOG_LEVEL >= PARTITION_LOG_ERROR
#define PART_ERR(format, ...) do{s_printf("[E]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define PART_ERR(format, ...)
#endif

struct partition_device
{
    char name[PARTITION_DEV_NAME_MAX];
    unsigned int size;
    unsigned int block_size;
    struct partition_dev_ops *ops;
    unsigned int id;
};

struct partition
{
    char name[PARTITION_NAME_MAX];
    unsigned int start;
    unsigned int size;
    struct partition_device *dev;
    unsigned int id;
};

static struct partition_device partition_dev_table[PARTITION_DEV_MAX] = {0};
static struct partition         partition_table[PARTITION_MAX]         = {0};

static unsigned int partition_dev_num = 0;
static unsigned int partition_num     = 0;

static unsigned int partition_strlen(const char *s)
{
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc)
    {
        /* nothing */
    }

    return sc - s;
}

static char *partition_strncpy(void *dst, const void *src, unsigned int n)
{
    if (n != 0U)
    {
        char *d = dst;
        const char *s = src;

        do
        {
            *d = *s;
            if (*s == 0)
            {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0U)
                {
                    *d++ = 0;
                }

                break;
            }
            d++;
            s++;
        } while (--n != 0U);
    }

    return (dst);
}

static int partition_strncmp(const char *cs, const char *ct, unsigned int n)
{
    signed char res = 0;

    while (n)
    {
        if ((res = *cs - *ct++) != 0 || !*cs++)
        {
            break;
        }

        n --;
    }

    return res;
}

static void *partition_memmove(void *dest, const void *src, unsigned int n)
{
    char *tmp = (char *)dest, *s = (char *)src;

    if ((s < tmp) && (tmp < (s + n)))
    {
        tmp += n;
        s += n;

        while (n--)
        {
             *(--tmp) = *(--s);
        }
    }
    else
    {
        while (n--)
        {
            *tmp++ = *s++;
        }
    }

    return dest;
}

static struct partition_device *partition_device_find(const char *dev_name)
{
    int i = 0;

    for (i = 0; i < partition_dev_num; i++)
    {
        if (0 == partition_strncmp(dev_name, partition_dev_table[i].name, partition_strlen(dev_name)))
        {
            PART_TRACE("partition device %s was found\r\n", dev_name);
            return &partition_dev_table[i];
        }
    }

    return PARTITION_NULL;
}

static struct partition *partition_find(const char *name)
{
    int i = 0;

    for (i = 0; i < partition_num; i++)
    {
        if (0 == partition_strncmp(name, partition_table[i].name, partition_strlen(name)))
        {
            PART_TRACE("partition %s was found\r\n", name);
            return &partition_table[i];
        }
    }

    return PARTITION_NULL;
}

int partition_device_register(const char *dev_name, struct partition_dev_ops *ops, unsigned int size, unsigned int block_size)
{
    if (dev_name == PARTITION_NULL || ops == PARTITION_NULL)
    {
        PART_ERR("partition device register err. name or ops is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    if (partition_dev_num >= PARTITION_DEV_MAX)
    {
        PART_ERR("partition device %s register err. number of device > PARTITION_DEV_MAX\r\n", dev_name);
        return PARTITION_ERR_FULL;
    }

    if (partition_device_find(dev_name) != PARTITION_NULL)
    {
        PART_ERR("partition device %s register err. device %s exist\r\n", dev_name, dev_name);
        return PARTITION_ERR_EXIST;
    }

    if (partition_strlen(dev_name) >= PARTITION_DEV_NAME_MAX)
    {
        partition_strncpy(partition_dev_table[partition_dev_num].name, dev_name, PARTITION_DEV_NAME_MAX);
    }
    else
    {
        partition_strncpy(partition_dev_table[partition_dev_num].name, dev_name, partition_strlen(dev_name));
    }

    partition_dev_table[partition_dev_num].size = size;
    partition_dev_table[partition_dev_num].block_size = block_size;
    partition_dev_table[partition_dev_num].ops = ops;
    partition_dev_table[partition_dev_num].id = partition_dev_num;
    partition_dev_num++;

    PART_INFO("partition device %s registered, size: 0x%x, block: 0x%x\r\n", dev_name, size, block_size);

    return PARTITION_OK;
}

int partition_device_unregister(const char *dev_name)
{
    int i = 0;
    struct partition_device *dev = PARTITION_NULL;

    if (dev_name == PARTITION_NULL)
    {
        PART_ERR("partition device unregister err. dev_name is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    dev = partition_device_find(dev_name);
    if (dev == PARTITION_NULL)
    {
        PART_ERR("partition device %s register err. device %s not exist\r\n", dev_name, dev_name);
        return PARTITION_ERR_NOEXIST;
    }

    for (i = 0; i < partition_dev_num; i++)
    {
        if (partition_table[i].dev == dev)
        {
            (void)partition_unregister(partition_table[i].name);
        }
    }

    partition_dev_num--;

    if (dev->id != (partition_dev_num))
    {
        partition_memmove(&partition_table[dev->id], &partition_dev_table[dev->id + 1], (partition_dev_num - dev->id) * sizeof(struct partition_device));
    }

    for (i = 0; i < PARTITION_DEV_NAME_MAX; i++)
    {
        partition_dev_table[partition_dev_num].name[i] = 0;
    }

   PART_INFO("partition device %s unregistered\r\n", dev_name);

    return PARTITION_OK;
}

int partition_device_init(const char *dev_name)
{
    int ret = PARTITION_ERR_UNKNOWN;
    struct partition_device *dev = PARTITION_NULL;

    if (dev_name == PARTITION_NULL)
    {
        PART_ERR("partition device init err. dev_name is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    dev = partition_device_find(dev_name);
    if (dev == PARTITION_NULL)
    {
        PART_ERR("partition device %s register err. device %s not exist\r\n", dev_name, dev_name);
        return PARTITION_ERR_NOEXIST;
    }

    if (dev->ops->init)
    {
        ret = dev->ops->init();
        PART_TRACE("partition device %s init done\r\n", dev_name);
    }

    return ret;
}

int partition_read(const char *partition_name, void *buf, unsigned int offset, unsigned int len)
{
    int ret = PARTITION_ERR_UNKNOWN;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL || buf == PARTITION_NULL)
    {
        PART_ERR("partition read err. partition_name or buf is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        PART_ERR("partition %s read err. partition %s not exist\r\n", partition_name, partition_name);
        return PARTITION_ERR_NOEXIST;
    }

    if ((offset + len) > part->size)
    {
        PART_WARN("partition %s read warn: out of range (offset=0x%x, len=0x%x, size=0x%x)\r\n", partition_name, offset, len, part->size);
        return PARTITION_ERR_SIZE;
    }

    addr = part->start + offset;

    if (part->dev->ops->read)
    {
        ret = part->dev->ops->read(addr, buf, len);
        PART_TRACE("partition %s read %u bytes at offset 0x%x\r\n", partition_name, len, offset);
    }

    return ret;
}

int partition_write(const char *partition_name, void *buf, unsigned int offset, unsigned int len)
{
    int ret = PARTITION_ERR_UNKNOWN;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL || buf == PARTITION_NULL)
    {
        PART_ERR("partition write err. partition_name or buf is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        PART_ERR("partition %s write err. partition %s not exist\r\n", partition_name, partition_name);
        return PARTITION_ERR_NOEXIST;
    }

    if ((offset + len) > part->size)
    {
        PART_WARN("partition %s write warn: out of range (offset=0x%x, len=0x%x, size=0x%x)\r\n", partition_name, offset, len, part->size);
        return PARTITION_ERR_SIZE;
    }

    addr = part->start + offset;

    if (part->dev->ops->write)
    {
        ret = part->dev->ops->write(addr, buf, len);
        PART_TRACE("partition %s write %u bytes to offset 0x%x\r\n", partition_name, len, offset);
    }

    return ret;
}

int partition_erase(const char *partition_name, unsigned int offset, unsigned int len)
{
    int ret = PARTITION_ERR_UNKNOWN;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL)
    {
        PART_ERR("partition erase err. partition_name is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        PART_ERR("partition %s erase err. partition %s not exist\r\n", partition_name, partition_name);
        return PARTITION_ERR_NOEXIST;
    }

    if ((offset + len) > part->size)
    {
        PART_WARN("partition %s erase warn: out of range (offset=0x%x, len=0x%x, size=0x%x)\r\n", partition_name, offset, len, part->size);
        return PARTITION_ERR_SIZE;
    }

    addr = part->start + offset;

    if (part->dev->ops->erase)
    {
        ret = part->dev->ops->erase(addr, len);
        PART_TRACE("partition %s erase %u bytes at offset 0x%x\r\n", partition_name, len, offset);
    }

    return ret;
}

int partition_erase_all(const char *partition_name)
{
    int ret = PARTITION_ERR_UNKNOWN;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL)
    {
        PART_ERR("partition erase all err. partition_name is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        PART_ERR("partition %s erase all err. partition %s not exist\r\n", partition_name, partition_name);
        return PARTITION_ERR_NOEXIST;
    }

    addr = part->start;

    if (part->dev->ops->erase)
    {
        ret = part->dev->ops->erase(addr, part->size);
        PART_TRACE("partition %s erase %u bytes\r\n", partition_name, part->size);
    }

    return ret;
}

int partition_register(const char *partition_name, const char *dev_name, unsigned int start, unsigned int size)
{
    struct partition_device *dev = PARTITION_NULL;

    if (partition_name == PARTITION_NULL || dev_name == PARTITION_NULL)
    {
        PART_ERR("partition register err. partition_name or dev_name is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    if (partition_num >= PARTITION_MAX)
    {
        PART_ERR("partition %s register err. number of partitoin > PARTITION_MAX\r\n", partition_name);
        return PARTITION_ERR_FULL;
    }

    dev = partition_device_find(dev_name);
    if (dev == PARTITION_NULL)
    {
        PART_ERR("partition %s register err. partition device %s is not exist\r\n", partition_name, dev_name);
        return PARTITION_ERR_NOEXIST;
    }

    if (partition_find(partition_name))
    {
        PART_ERR("partition %s register err. partition %s exist\r\n", partition_name, partition_name);
        return PARTITION_ERR_EXIST;
    }

    if ((start + size) > dev->size)
    {
        PART_ERR("partition %s register err. size out of range (start=0x%x, size=0x%x, dev size=0x%x)\r\n", partition_name, start, size, dev->size);
        return PARTITION_ERR_SIZE;
    }

    if (partition_strlen(partition_name) >= PARTITION_NAME_MAX)
    {
        partition_memmove(partition_table[partition_num].name, partition_name, PARTITION_NAME_MAX);
    }
    else
    {
        partition_memmove(partition_table[partition_num].name, partition_name, partition_strlen(partition_name));
    }

    partition_table[partition_num].dev = dev;
    partition_table[partition_num].start = start;
    partition_table[partition_num].size = size;
    partition_table[partition_num].id = partition_num;
    partition_num++;

    PART_INFO("partition %s register on device %s at 0x%08x size 0x%08x\r\n", partition_name, dev_name, start, size);

    return PARTITION_OK;
}

int partition_unregister(const char *partition_name)
{
    int i = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL)
    {
        PART_ERR("partition unregister err. partition_name is NULL\r\n");
        return PARTITION_ERR_PARAM;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        PART_ERR("partition %s unregister err. partition %s is not exist\r\n", partition_name, partition_name);
        return PARTITION_ERR_NOEXIST;
    }

    partition_num--;

    if (part->id != (partition_num))
    {
        partition_memmove(&partition_table[part->id], &partition_table[part->id + 1], (partition_num - part->id) * sizeof(struct partition));
    }

    for (i = 0; i < PARTITION_NAME_MAX; i++)
    {
        partition_table[partition_num].name[i] = 0;
    }

    PART_INFO("partition %s unregistered\r\n", partition_name);

    return PARTITION_OK;
}

void show_partition_info(void)
{
    int i = 0;

    PART_INFO("\r\npartition info : \r\n");
    PART_INFO("| name             | start      | size       | device           |\r\n");
    PART_INFO("| ---------------- | ---------- | ---------- | ---------------- |\r\n");
    for (i = 0; i < partition_num; i++)
    {
        PART_INFO("| %-16s | 0x%08x | 0x%08x | %-16s |\r\n", partition_table[i].name, partition_table[i].start, partition_table[i].size, partition_table[i].dev->name);
    }
}
