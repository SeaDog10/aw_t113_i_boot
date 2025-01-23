#include "partition.h"

static slist_t partition_dev_table_list;
static slist_t partition_table_list;

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

static void slist_init(slist_t *l)
{
    l->next = NULL;
}

static void slist_append(slist_t *l, slist_t *n)
{
    struct slist_node *node;

    node = l;
    while (node->next) node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next = NULL;
}

static void slist_insert(slist_t *l, slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

static unsigned int slist_len(const slist_t *l)
{
    unsigned int len = 0;
    const slist_t *list = l->next;
    while (list != NULL)
    {
        list = list->next;
        len ++;
    }

    return len;
}

static slist_t *slist_remove(slist_t *l, slist_t *n)
{
    /* remove slist head */
    struct slist_node *node = l;
    while (node->next && node->next != n) node = node->next;

    /* remove node */
    if (node->next != (slist_t *)0) node->next = node->next->next;

    return l;
}

static slist_t *slist_first(slist_t *l)
{
    return l->next;
}

static slist_t *slist_tail(slist_t *l)
{
    while (l->next) l = l->next;

    return l;
}

static slist_t *slist_next(slist_t *n)
{
    return n->next;
}

static int slist_isempty(slist_t *l)
{
    return l->next == NULL;
}

int partition_device_register(struct partition_device *dev, const char *dev_name)
{
    int ret = 0;

    if (dev == NULL || dev_name == NULL)
    {
        ret = -1;
        goto exit;
    }

    if (strlen(dev_name) >= PARTITION_NAME_MAX)
    {
        memcpy(dev->name, dev_name, PARTITION_NAME_MAX);
    }
    else
    {
        memcpy(dev->name, dev_name, strlen(dev_name));
    }

    if (partition_device_find(dev->name) != NULL)
    {
        ret = -1;
        goto exit;
    }

    slist_init(&dev->node);
    slist_insert(&partition_dev_table_list, &dev->node);

exit:
    return ret;
}

int partition_device_unregister(const char *dev_name)
{
    int ret = 0;
    struct partition_device *dev = NULL;

    if (dev_name == NULL)
    {
        ret = -1;
        goto exit;
    }

    dev = partition_device_find(dev_name);
    if (dev == NULL)
    {
        ret = -1;
        goto exit;
    }

    slist_remove(&partition_dev_table_list, &dev->node);

exit:
    return ret;
}

struct partition_device *partition_device_find(const char *dev_name)
{
    int i = 0;
    struct partition_device *dev = NULL;

    if (dev_name == NULL)
    {
        goto exit;
    }

    for (i = 0; i < slist_len(&partition_dev_table_list); i++)
    {
        dev = container_of(slist_next(&partition_dev_table_list), struct partition_device, node);
        if (0 == strcmp(dev_name, dev->name))
        {
            break;
        }
    }

exit:
    return dev;
}

/**
 * @brief 初始化分区设备
 *
 * @param dev 分区设备指针
 * @return int 初始化结果，成功返回0，失败返回-1
 */

int partition_device_init(struct partition_device *dev)
{
    int ret = -1;

    if (dev == NULL)
    {
        ret = -1;
        goto exit;
    }

    if (dev->ops.init)
    {
        ret = dev->ops.init();
    }

exit:
    return ret;
}

/**
 * @brief 在分区设备中查找指定名称的分区
 *
 * @param name 要查找的分区名称
 * @return struct partition* 找到的分区指针，如果未找到则返回NULL
 */
struct partition *partition_find(const char *name)
{
    int i = 0;
    struct partition *part = NULL;

    if (name == NULL)
    {
        return NULL;
    }

    for (i = 0; i < slist_len(&partition_table_list); i++)
    {
        part = container_of(slist_next(&partition_table_list), struct partition, node);
        if (0 == strcmp(name, part->name))
        {
            break;
        }
    }

    return part;
}


int partition_read(struct partition *part, void *buf, unsigned int offset, unsigned int len)
{
    int ret = -1;

    if (part == NULL || buf == NULL)
    {
        ret = -1;
        goto exit;
    }

    if (offset + len > part->size)
    {
        ret = -1;
        goto exit;
    }

    if (part->dev->ops.read)
    {
        ret = part->dev->ops.read(buf, offset, len);
    }

exit:
    return ret;
}

int partition_write(struct partition *part, void *buf, unsigned int offset, unsigned int len)
{
    int ret = -1;

    if (part == NULL || buf == NULL)
    {
        ret = -1;
        goto exit;
    }

    if (offset + len > part->size)
    {
        ret = -1;
        goto exit;
    }

    if (part->dev->erase_before_write)
    {
        ret = part->dev->ops.erase(offset, len);
        if (ret != 0)
        {
            goto exit;
        }
    }

    if (part->dev->ops.write)
    {
        ret = part->dev->ops.write(buf, offset, len);
    }

exit:
    return ret;
}

int partition_erase(struct partition *part, unsigned int offset, unsigned int len)
{
    int ret = -1;

    if (part == NULL)
    {
        ret = -1;
        goto exit;
    }

    if (offset + len > part->size)
    {
        ret = -1;
        goto exit;
    }

    if (part->dev->ops.erase)
    {
        ret = part->dev->ops.erase(offset, len);
    }

exit:
    return ret;
}

int partition_register(struct partition *part, const char *dev_name, const char *part_name)
{
    int ret = 0;
    struct partition_device *dev = NULL;

    if (part == NULL || dev_name == NULL || part_name == NULL)
    {
        ret = -1;
        goto exit;
    }

    dev = partition_device_find(dev_name);
    if (dev == NULL)
    {
        ret = -1;
        goto exit;
    }

    if (partition_find(part_name))
    {
        ret = -1;
        goto exit;
    }

    if (strlen(part_name) >= PARTITION_NAME_MAX)
    {
        memcpy(part->name, part_name, PARTITION_NAME_MAX);
    }
    else
    {
        memcpy(part->name, part_name, strlen(part_name));
    }

    if ((part->start + part->size) > dev->size)
    {
        ret = -1;
        goto exit;
    }

    slist_init(&part->node);
    slist_insert(&partition_table_list, &part->node);

exit :
    return ret;
}

int partition_unregister(const char *name)
{
    int ret = 0;
    struct partition *part = NULL;

    if (name == NULL)
    {
        ret = -1;
        goto exit;
    }

    part = partition_find(name);
    if (part == NULL)
    {
        ret = -1;
        goto exit;
    }

    list_remove(&partition_table_list, &part->node);

exit:
    return ret;
}
