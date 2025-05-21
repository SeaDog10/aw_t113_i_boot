#include "partition.h"

static slist_t partition_dev_table_list =
{
    .next = NULL
};

static slist_t partition_table_list =
{
    .next = NULL
};

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

static slist_t *slist_next(slist_t *n)
{
    return n->next;
}

/**
 * @brief 注册一个分区设备到分区设备列表中
 *
 * @param dev 要注册的分区设备指针
 * @param dev_name 分区设备的名称
 * @return int 注册操作的结果，成功返回0，失败返回-1
 */
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
    slist_append(&partition_dev_table_list, &dev->node);

exit:
    return ret;
}

/**
 * @brief 从分区设备列表中注销指定名称的分区设备
 *
 * @param dev_name 要注销的分区设备名称
 * @return int 注销操作的结果，成功返回0，失败返回-1
 */
int partition_device_unregister(const char *dev_name)
{
    int ret = 0;
    slist_t *node = NULL;
    struct partition_device *dev = NULL;
    struct partition *part = NULL;

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

    node = slist_next(&partition_table_list);
    while (node != NULL)
    {
        part = container_of(node, struct partition, node);
        if (dev == part->dev)
        {
            slist_remove(&partition_table_list, node);
        }
        node = node->next;
    }

    slist_remove(&partition_dev_table_list, &dev->node);

exit:
    return ret;
}

/**
 * @brief 在分区设备列表中查找指定名称的分区设备
 *
 * @param dev_name 要查找的分区设备名称
 * @return struct partition_device* 找到的分区设备指针，如果未找到则返回NULL
 */
struct partition_device *partition_device_find(const char *dev_name)
{
    slist_t *node = NULL;
    struct partition_device *dev = NULL;

    if (dev_name == NULL)
    {
        goto exit;
    }

    node = slist_next(&partition_dev_table_list);
    while (node != NULL)
    {
        dev = container_of(node, struct partition_device, node);
        if (0 == strcmp(dev_name, dev->name))
        {
            break;
        }
        node = node->next;
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
    slist_t *node = NULL;
    struct partition *part = NULL;

    if (name == NULL)
    {
        return NULL;
    }

    node = slist_next(&partition_table_list);
    while (node != NULL)
    {
        part = container_of(node, struct partition, node);
        if (0 == strcmp(name, part->name))
        {
            return part;
        }
        node = node->next;
    }

    return NULL;
}

/**
 * @brief 从指定分区读取数据
 *
 * @param part 分区指针
 * @param buf 数据缓冲区指针
 * @param offset 读取数据的偏移量
 * @param len 要读取的数据长度
 * @return int 读取操作的结果，成功返回0，失败返回-1
 */
int partition_read(struct partition *part, void *buf, unsigned int offset, unsigned int len)
{
    int ret = -1;
    unsigned int addr = 0;

    if (part == NULL || buf == NULL)
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

    if (part->dev->ops.read)
    {
        ret = part->dev->ops.read(addr, buf, len);
    }

exit:
    return ret;
}

/**
 * @brief 向指定分区写入数据
 *
 * @param part 分区指针
 * @param buf 数据缓冲区指针
 * @param offset 写入数据的偏移量
 * @param len 要写入的数据长度
 * @return int 写入操作的结果，成功返回0，失败返回-1
 */
int partition_write(struct partition *part, void *buf, unsigned int offset, unsigned int len)
{
    int ret = -1;
    unsigned int addr = 0;

    if (part == NULL || buf == NULL)
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

    if (part->dev->erase_before_write)
    {
        ret = part->dev->ops.erase(addr, len);
        if (ret != 0)
        {
            goto exit;
        }
    }

    if (part->dev->ops.write)
    {
        ret = part->dev->ops.write(addr, buf, len);
    }

exit:
    return ret;
}

/**
 * @brief 擦除指定分区中的数据
 *
 * @param part 分区指针
 * @param offset 擦除数据的偏移量
 * @param len 要擦除的数据长度
 * @return int 擦除操作的结果，成功返回0，失败返回-1
 */
int partition_erase(struct partition *part, unsigned int offset, unsigned int len)
{
    int ret = -1;
    unsigned int addr = 0;

    if (part == NULL)
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

    if (part->dev->ops.erase)
    {
        ret = part->dev->ops.erase(addr, len);
    }

exit:
    return ret;
}

/**
 * @brief 注册一个分区到分区列表中
 *
 * @param part 要注册的分区指针
 * @param dev_name 分区所属的设备名称
 * @param part_name 分区的名称
 * @return int 注册操作的结果，成功返回0，失败返回-1
 */
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

    if ((part->size) > dev->size)
    {
        ret = -1;
        goto exit;
    }

    part->dev = dev;
    slist_init(&part->node);
    slist_append(&partition_table_list, &part->node);

exit :
    return ret;
}

/**
 * @brief 从分区列表中注销指定名称的分区
 *
 * @param name 要注销的分区名称
 * @return int 注销操作的结果，成功返回0，失败返回-1
 */
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

    slist_remove(&partition_table_list, &part->node);

exit:
    return ret;
}

#if 1
#include "debug.h"

void show_partition_info(void)
{
    slist_t *node = NULL;
    struct partition *part = NULL;

    node = slist_next(&partition_table_list);
    debug("partition info : \r\n");
    debug("| name             | start      | size       | device           |\r\n");
    debug("| ---------------- | ---------- | ---------- | ---------------- |\r\n");
    while (node!= NULL)
    {
        part = container_of(node, struct partition, node);
        debug("| %-16s | 0x%08x | 0x%08x | %-16s |\r\n", part->name, part->start, part->size, part->dev->name);
        node = node->next;
    }
}
#endif
