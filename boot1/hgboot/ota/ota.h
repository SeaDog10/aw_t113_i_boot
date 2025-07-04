#ifndef __OTA_H__
#define __OTA_H__

#include "partition/partition.h"
#include "ymodem/ymodem.h"

#define OTA_FIRMWARE_MAGIC 0x46574D47    /* Magic number for firmware header ('FWMG') */
#define OTA_PARA_MAGIC     0x50415241    /* Magic number for OTA parameter ('PARA') */

#define CACHE_SIZE 1024                  /* Size of the cache buffer used during OTA operations (in bytes) */

#define DOWN_PART  "Download"            /* Partition name for firmware download */
#define APP1_PART  "APP1"                /* Partition name for application slot 1 */
#define APP2_PART  "APP2"                /* Partition name for application slot 2 */
#define PARA_PART  "Param"               /* Partition name for OTA parameters */

#define OTA_LOG_NONE     0               /* OTA log level: no output */
#define OTA_LOG_ERROR    1               /* OTA log level: error output */
#define OTA_LOG_WARN     2               /* OTA log level: warning output */
#define OTA_LOG_INFO     3               /* OTA log level: informational output */
#define OTA_LOG_DEBUG    4               /* OTA log level: debug output */
#define OTA_LOG_TRACE    5               /* OTA log level: trace output */

#define OTA_LOG_LEVEL    YMODEM_LOG_ERROR/* Set the current OTA log level */

#define APP_SLOT_1       0xa1U           /* Application slot 1 index */
#define APP_SLOT_2       0xa2U           /* Application slot 2 index */

/**
 * @enum ota_errcode_t
 * OTA error code definitions for OTA process status and error handling.
 */
typedef enum
{
    OTA_OK              =  0,   /* Operation successful */
    OTA_ERR_DOWNLOAD    = -1,   /* Download failed */
    OTA_ERR_UPDATE      = -2,   /* Update failed */
    OTA_ERR_BACKUP      = -3,   /* Backup failed */
    OTA_ERR_CHECK       = -4,   /* Check failed (CRC, magic, etc.) */
    OTA_ERR_PARTITION   = -5,   /* Partition operation failed */
} ota_errcode_t;

/**
 * @struct firmware_header
 * Firmware image header information, used for validation and loading.
 */
struct firmware_header
{
    unsigned int magic;        /* Magic number for firmware header */
    unsigned int size;         /* Firmware image size in bytes */
    unsigned int crc32;        /* CRC32 checksum of firmware image */
    unsigned int version;      /* Firmware version */
    unsigned int load_addr;    /* Address to load firmware */
    unsigned int exec_addr;    /* Firmware execution entry address */
    unsigned int reseved[2];   /* Reserved for future use */
};

/**
 * @struct ota_paramers
 * OTA parameter structure, stores OTA status and CRCs for slots.
 */
struct ota_paramers
{
    unsigned int magic;         /* Magic number for OTA parameter */
    unsigned int active_slot;   /* Currently active firmware slot */
    unsigned int can_be_back;   /* Whether backup/rollback is allowed */
    unsigned int upgrade_ready; /* Whether upgrade is ready */
    unsigned int app1_crc;      /* CRC32 of APP1 slot */
    unsigned int app2_crc;      /* CRC32 of APP2 slot */
    unsigned int download_crc;  /* CRC32 of downloaded firmware */
    unsigned int reserved;      /* Reserved for future use */
};

int ota_download_firmware(void);
int ota_update_firmware(void);
int ota_backup_firmware(void);

#endif /* __OTA_H__ */
