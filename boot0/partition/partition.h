#ifndef _PARTITION_H_
#define _PARTITION_H_

#include "string.h"
#include "stdio.h"

#define PARTITION_NAME_MAX   16

typedef struct slist_node
{
    struct slist_node *next;                          /**< point to next node. */
}slist_t;

struct partition_ops
{
    int (*init) (void);
    int (*read) (unsigned int addr, unsigned char *buf, unsigned int size);
    int (*write)(unsigned int addr, const unsigned char  *buf, unsigned int size);
    int (*erase)(unsigned int addr, unsigned int size);
};

struct partition_device
{
    char name[PARTITION_NAME_MAX];
    unsigned int size;
    unsigned int block_size;
    int erase_before_write;
    struct partition_ops ops;
    slist_t node;
};

struct partition
{
    struct partition_device *dev;
    char name[PARTITION_NAME_MAX];
    unsigned int start;
    unsigned int size;
    slist_t node;
};

int partition_device_register(struct partition_device *dev, const char *dev_name);
int partition_device_unregister(const char *dev_name);
struct partition_device *partition_device_find(const char *dev_name);
int partition_device_init(struct partition_device *dev);

int partition_register(struct partition *part, const char *dev_name, const char *part_name);
int partition_unregister(const char *name);
struct partition *partition_find(const char *name);
int partition_read(struct partition *part, void *buf, unsigned int offset, unsigned int len);
int partition_write(struct partition *part, void *buf, unsigned int offset, unsigned int len);
int partition_erase(struct partition *part, unsigned int offset, unsigned int len);

void show_partition_info(void);

#endif
