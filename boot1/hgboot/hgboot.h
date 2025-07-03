#ifndef __HG_BOOT_H__
#define __HG_BOOT_H__

#include "ota/ota.h"
#include "partition/partition.h"

#define BOOT_MAX_RETRY    5

#define BOOT_LOG_NONE     0
#define BOOT_LOG_ERROR    1
#define BOOT_LOG_WARN     2
#define BOOT_LOG_INFO     3
#define BOOT_LOG_DEBUG    4
#define BOOT_LOG_TRACE    5

#define BOOT_LOG_LEVEL    BOOT_LOG_ERROR

void *boot_firmware(void);

#endif
