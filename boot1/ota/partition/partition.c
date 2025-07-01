#include "partition.h"

#define PARTITION_NULL            0

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
    struct partition_device *dev = PARTITION_NULL;

    for (i = 0; i < partition_dev_num; i++)
    {
        if (0 == partition_strncmp(dev_name, partition_dev_table[i].name, partition_strlen(dev_name)))
        {
            dev = &partition_dev_table[i];
            break;
        }
    }

    return dev;
}

static struct partition *partition_find(const char *name)
{
    int i = 0;
    struct partition *part = PARTITION_NULL;

    for (i = 0; i < partition_num; i++)
    {
        if (0 == partition_strncmp(name, partition_table[i].name, partition_strlen(name)))
        {
            part = &partition_table[i];
            break;
        }
    }

    return part;
}

int partition_device_register(const char *dev_name, struct partition_dev_ops *ops, unsigned int size, unsigned int block_size)
{
    int ret = 0;

    if (dev_name == PARTITION_NULL || ops == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    if (partition_dev_num >= PARTITION_DEV_MAX)
    {
        ret = -1;
        goto exit;
    }

    if (partition_device_find(dev_name) != PARTITION_NULL)
    {
        ret = -1;
        goto exit;
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

exit:
    return ret;
}

int partition_device_unregister(const char *dev_name)
{
    int ret = -1;
    int i = 0;
    struct partition_device *dev = PARTITION_NULL;

    if (dev_name == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    dev = partition_device_find(dev_name);
    if (dev == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
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

exit:
    return ret;
}

int partition_device_init(const char *dev_name)
{
    int ret = -1;
    struct partition_device *dev = PARTITION_NULL;

    if (dev_name == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    dev = partition_device_find(dev_name);
    if (dev == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    if (dev->ops->init)
    {
        ret = dev->ops->init();
    }

exit:
    return ret;
}

int partition_read(const char *partition_name, void *buf, unsigned int offset, unsigned int len)
{
    int ret = -1;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL || buf == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    if ((offset + len) > part->size)
    {
        ret = -1;
        goto exit;
    }

    addr = part->start + offset;

    if (part->dev->ops->read)
    {
        ret = part->dev->ops->read(addr, buf, len);
    }

exit:
    return ret;
}

int partition_write(const char *partition_name, void *buf, unsigned int offset, unsigned int len)
{
    int ret = -1;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL || buf == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    if ((offset + len) > part->size)
    {
        ret = -1;
        goto exit;
    }

    addr = part->start + offset;

    if (part->dev->ops->write)
    {
        ret = part->dev->ops->write(addr, buf, len);
    }

exit:
    return ret;
}

int partition_erase(const char *partition_name, unsigned int offset, unsigned int len)
{
    int ret = -1;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    if ((offset + len) > part->size)
    {
        ret = -1;
        goto exit;
    }

    addr = part->start + offset;

    if (part->dev->ops->erase)
    {
        ret = part->dev->ops->erase(addr, len);
    }

exit:
    return ret;
}

int partition_erase_all(const char *partition_name)
{
    int ret = -1;
    unsigned int addr = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    addr = part->start;

    if (part->dev->ops->erase)
    {
        ret = part->dev->ops->erase(addr, part->size);
    }

exit:
    return ret;
}

int partition_register(const char *partition_name, const char *dev_name, unsigned int start, unsigned int size)
{
    int ret = 0;
    struct partition_device *dev = PARTITION_NULL;

    if (partition_name == PARTITION_NULL || dev_name == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    if (partition_num >= PARTITION_MAX)
    {
        ret = -1;
        goto exit;
    }

    dev = partition_device_find(dev_name);
    if (dev == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    if (partition_find(partition_name))
    {
        ret = -1;
        goto exit;
    }

    if ((start + size) > dev->size)
    {
        ret = -1;
        goto exit;
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

exit :
    return ret;
}

int partition_unregister(const char *partition_name)
{
    int ret = 0;
    int i = 0;
    struct partition *part = PARTITION_NULL;

    if (partition_name == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
    }

    part = partition_find(partition_name);
    if (part == PARTITION_NULL)
    {
        ret = -1;
        goto exit;
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

exit:
    return ret;
}

#if 1
#include "shell/shell.h"

void show_partition_info(void)
{
    int i = 0;

    s_printf("partition info : \r\n");
    s_printf("| name             | start      | size       | device           |\r\n");
    s_printf("| ---------------- | ---------- | ---------- | ---------------- |\r\n");
    for (i = 0; i < partition_num; i++)
    {
        s_printf("| %-16s | 0x%08x | 0x%08x | %-16s |\r\n", partition_table[i].name, partition_table[i].start, partition_table[i].size, partition_table[i].dev->name);
    }
}
#endif
