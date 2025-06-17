#ifndef __LITTLEFS_H__
#define __LITTLEFS_H__

#include "board.h"
#include "drv_nand.h"
#include "lfs.h"

#define PAGE_OFFSET_OF_NAND     2560    /* block_start: 40 block_end 72 */

extern lfs_t nand_lfs;

int drv_lfs_mount(void);

#endif
