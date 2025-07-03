#ifndef _PARTITION_H_
#define _PARTITION_H_

#define PARTITION_DEV_NAME_MAX    16    /* Maximum length of partition device name (including null terminator) */
#define PARTITION_NAME_MAX        16    /* Maximum length of partition name (including null terminator) */
#define PARTITION_DEV_MAX         5     /* Maximum number of partition devices supported */
#define PARTITION_MAX             20    /* Maximum number of partitions supported */

#define PARTITION_LOG_NONE     0        /* Partition log level: no output */
#define PARTITION_LOG_ERROR    1        /* Partition log level: error output */
#define PARTITION_LOG_WARN     2        /* Partition log level: warning output */
#define PARTITION_LOG_INFO     3        /* Partition log level: informational output */
#define PARTITION_LOG_DEBUG    4        /* Partition log level: debug output */
#define PARTITION_LOG_TRACE    5        /* Partition log level: trace output */

#define PARTITION_LOG_LEVEL    PARTITION_LOG_ERROR    /* Set the current partition log level */

/* Error codes for partition operations. */
typedef enum
{
    PARTITION_OK          =  0,  /* Operation successful */
    PARTITION_ERR_PARAM   = -1,  /* Invalid parameter */
    PARTITION_ERR_SIZE    = -2,  /* Size error */
    PARTITION_ERR_FULL    = -3,  /* Partition table full */
    PARTITION_ERR_EXIST   = -4,  /* Partition/device already exists */
    PARTITION_ERR_NOEXIST = -5,  /* Partition/device does not exist */
    PARTITION_ERR_UNKNOWN = -6,  /* Unknown error */
} partition_errcode_t; /* partition_errcode_t: Error codes for partition operations. */

/* Device operation structure for partition device abstraction. Users should implement these low-level functions for their storage device. */
typedef struct partition_dev_ops
{
    int (*init) (void);                                              /* Initialize the device */
    int (*read) (unsigned int addr, unsigned char *buf, unsigned int size);   /* Read data from device */
    int (*write)(unsigned int addr, unsigned char  *buf, unsigned int size);  /* Write data to device */
    int (*erase)(unsigned int addr, unsigned int size);                      /* Erase data on device */
} partition_dev_ops_t; /* partition_dev_ops_t: Device operation structure for partition device abstraction. */

int partition_device_register(const char *dev_name, struct partition_dev_ops *ops, unsigned int size);
int partition_device_unregister(const char *dev_name);
int partition_device_init(const char *dev_name);

int partition_register(const char *partition_name, const char *dev_name, unsigned int start, unsigned int size);
int partition_unregister(const char *partition_name);

int partition_read(const char *partition_name, void *buf, unsigned int offset, unsigned int len);
int partition_write(const char *partition_name, void *buf, unsigned int offset, unsigned int len);
int partition_erase(const char *partition_name, unsigned int offset, unsigned int len);
int partition_erase_all(const char *partition_name);

void show_partition_info(void);

#endif /* _PARTITION_H_ */
