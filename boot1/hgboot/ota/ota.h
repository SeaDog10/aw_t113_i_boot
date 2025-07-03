#ifndef __OTA_H__
#define __OTA_H__

#include "partition/partition.h"
#include "ymodem/ymodem.h"

#define OTA_FIRMWARE_MAGIC 0x46574D47  // 'FWMG'
#define OTA_PARA_MAGIC     0x50415241  // 'PARA'

#define CACHE_SIZE 1024

#define DOWN_PART  "Download"
#define APP1_PART  "APP1"
#define APP2_PART  "APP2"
#define PARA_PART  "Param"

#define OTA_LOG_NONE     0
#define OTA_LOG_ERROR    1
#define OTA_LOG_WARN     2
#define OTA_LOG_INFO     3
#define OTA_LOG_DEBUG    4
#define OTA_LOG_TRACE    5

#define OTA_LOG_LEVEL    YMODEM_LOG_ERROR

#define APP_SLOT_1       0U
#define APP_SLOT_2       1U

typedef enum
{
    OTA_OK              =  0,
    OTA_ERR_DOWNLOAD    = -1,
    OTA_ERR_UPDATE      = -2,
    OTA_ERR_BACKUP      = -3,
    OTA_ERR_CHECK       = -4,
    OTA_ERR_PARTITION   = -5,
}ota_errcode_t;

struct firmware_header
{
    unsigned int magic;
    unsigned int size;
    unsigned int crc32;
    unsigned int version;
    unsigned int load_addr;
    unsigned int exec_addr;
    unsigned int reseved[2];
};

struct ota_paramers
{
    unsigned int magic;
    unsigned int active_slot;
    unsigned int can_be_back;
    unsigned int upgrade_ready;
    unsigned int app1_crc;
    unsigned int app2_crc;
    unsigned int download_crc;
    unsigned int reserved;
};

int ota_download_firmware(void);
int ota_update_firmware(void);
int ota_backup_firmware(void);

#endif
