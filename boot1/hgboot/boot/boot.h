#ifndef __BOOT_H__
#define __BOOT_H__

#include "ota/ota.h"
#include "partition/partition.h"

#define BOOT_MAX_RETRY    5    /* The maximum number of retries for booting firmware */

/* Boot log level definitions */
#define BOOT_LOG_NONE     0    /* No logging output */
#define BOOT_LOG_ERROR    1    /* Error logging output */
#define BOOT_LOG_WARN     2    /* Warning logging output */
#define BOOT_LOG_INFO     3    /* Informational logging output */
#define BOOT_LOG_DEBUG    4    /* Debug logging output */
#define BOOT_LOG_TRACE    5    /* Trace logging output */

/* Set the current boot log level */
#define BOOT_LOG_LEVEL    BOOT_LOG_ERROR    /* Current log level for boot process */

void *boot_firmware(void);

#endif /* __BOOT_H__ */
