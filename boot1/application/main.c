#include "main.h"
#include "sunxi_gpio.h"
#include "sunxi_clk.h"
#include "sunxi_dma.h"
#include "arm32.h"
#include "reg-ccu.h"
#include "debug.h"
#include "board.h"
#include "barrier.h"
#include "memheap.h"
#include "drv_nand.h"
#include "string.h"

# if 0
static void hexdump(const void *p, uint32_t len)
{
    unsigned char *buf = (unsigned char *)p;
    int i = 0, j=  0;

    message("Dump %p %dBytes\r\n", p, len);
    message("Offset      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");

    for (i=0; i<len; i+=16)
    {
        message("%p: ", &buf[i+j]);

        for (j=0; j<16; j++)
        {
            if (i+j < len)
            {
                message("%02X ", buf[i+j]);
            }
            else
            {
                message("   ");
            }
        }
        message(" ");

        for (j=0; j<16; j++)
        {
            if (i+j < len)
            {
                message("%c", ((unsigned int)((buf[i+j]) - ' ') < 127u - ' ') ? buf[i+j] : '.');
            }
        }
        message("\r\n");
    }
}
#endif

int main(void)
{
    int32_t ret = 0;
    uint32_t i = 0;
    uint32_t total, used, maxused;
    void (*app_entry)(void);
    uint8_t *dst_addr = 0x40020000;

    app_entry = (void (*)(void))0x40020000;

    gpio_t led = GPIO_PIN(PORTC, 0);

    board_init();
    sunxi_clk_init();

#ifndef BUILD_FOR_DEBUG
    sunxi_dram_init();
#endif

    sunxi_clk_dump();

    dma_init();

    debug("__memheap_start : 0x%08x, __memheap_end : 0x%08x\r\n", (uint32_t)HEAP_BEGIN, (uint32_t)HEAP_END);
    ret = rt_memheap_init(&system_heap, HEAP_BEGIN, HEAP_SIZE);
    if (ret != 0)
    {
        error("memheap init failed\r\n");
    }
    else
    {
        rt_memheap_info(&system_heap, &total, &used, &maxused);
        debug("memheap init success, total: 0x%08x, used: 0x%08x, maxused: 0x%08x\r\n", total, used, maxused);
    }

    debug("SPI: init\r\n");
    if (sunxi_spi_init(&sunxi_spi0) != 0)
    {
        error("SPI: init failed\r\n");
    }

    ret = nand_init(&nand_flash, &sunxi_spi0);
    if (ret == 0)
    {
        debug("NAND: init ok\r\n");
    }

    debug("Hello hg'boot\r\n");

    for (i = 0; i < 512; i++)
    {
        nand_read_page(&nand_flash, 512 + i, dst_addr + (nand_flash.page_size * i), 2048, NULL, 0);
    }

    info("booting rtt...\r\n");

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();
	arm32_interrupt_disable();

    app_entry();

    return 0;

    // while(1)
    // {
    //     sunxi_gpio_set_value(led, 0);
    //     mdelay(500);
    //     sunxi_gpio_set_value(led, 1);
    //     mdelay(500);
    // }
}