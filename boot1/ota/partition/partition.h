#ifndef _PARTITION_H_
#define _PARTITION_H_

#define PARTITION_DEV_NAME_MAX    16
#define PARTITION_NAME_MAX        16
#define PARTITION_DEV_MAX         5
#define PARTITION_MAX             20

struct partition_dev_ops
{
    int (*init) (void);
    int (*read) (unsigned int addr, unsigned char *buf, unsigned int size);
    int (*write)(unsigned int addr, unsigned char  *buf, unsigned int size);
    int (*erase)(unsigned int addr, unsigned int size);
};

int partition_device_register(const char *dev_name, struct partition_dev_ops *ops, unsigned int size, unsigned int block_size);
int partition_device_unregister(const char *dev_name);
int partition_device_init(const char *dev_name);

int partition_register(const char *partition_name, const char *dev_name, unsigned int start, unsigned int size);
int partition_unregister(const char *partition_name);

int partition_read(const char *partition_name, void *buf, unsigned int offset, unsigned int len);
int partition_write(const char *partition_name, void *buf, unsigned int offset, unsigned int len);
int partition_erase(const char *partition_name, unsigned int offset, unsigned int len);
int partition_erase_all(const char *partition_name);

void show_partition_info(void);

#endif
