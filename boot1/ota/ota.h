#ifndef __OTA_H__
#define __OTA_H__

#include "partition/partition.h"
#include "ymodem/ymodem.h"
#include "shell/shell.h"

#define OTA_MAGIC          0x46574D47  // 'FWMG'
#define OTA_PARA_MAGIC     0x50415241  // 'PARA'

#define CHUNK_SIZE 1024

#define DOWN_PART  "Download"
#define APP1_PART  "APP1"
#define APP2_PART  "APP2"
#define PARA_PART  "Param"

typedef void (*app_entry_t)(void);

int ota_download(void);
app_entry_t ota_boot(void);

#endif
