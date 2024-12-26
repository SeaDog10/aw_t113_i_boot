#ifndef __BOARD_H__
#define __BOARD_H__

#include "dram.h"
#include "sunxi_spi.h"
#include "sunxi_usart.h"
#include "sunxi_sdhci.h"

#define USART_DBG usart0_dbg
#define CONFIG_ENABLE_CPU_FREQ_DUMP

#define CONFIG_CPU_FREQ 1200000000

extern dram_para_t   ddr_param;
extern sunxi_usart_t USART_DBG;
extern sunxi_spi_t   sunxi_spi0;

extern void board_init(void);
extern int  board_sdhci_init(void);

#endif
