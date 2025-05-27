#include "main.h"
#include "sunxi_gpio.h"
#include "sunxi_clk.h"
#include "sunxi_dma.h"
#include "arm32.h"
#include "reg-ccu.h"
#include "debug.h"
#include "board.h"
#include "barrier.h"
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

#define IMG_OFFSET_IN_FLASH 0x100000
#define IMG_MAGIC 0x12345678

typedef struct boot_head
{
    uint32_t img_magic;
    uint32_t img_size;
    uint32_t img_load;
    uint32_t img_entry;
}boot_head_t;

int main(void)
{
    uint32_t i = 0;
    uint32_t count = 0;
    void (*app_entry)(void);
    uint8_t *dst_addr;
    boot_head_t img_head;

    board_init();
    sunxi_clk_init();
    sunxi_dram_init();
    dma_init();

    debug("SPI: init\r\n");
    if (sunxi_spi_init(&sunxi_spi0) != 0)
    {
        error("SPI: init failed\r\n");
        while(1);
    }

    if (spi_nand_detect(&sunxi_spi0) != 0)
    {
        error("SPI: nand detect failed\r\n");
        while(1);
    }

    spi_nand_read(&sunxi_spi0, (uint8_t *)&img_head, IMG_OFFSET_IN_FLASH, sizeof(boot_head_t));

    if (img_head.img_magic != IMG_MAGIC)
    {
        error("img_magic err\r\n");
        while(1);
    }

    debug("img_magic: 0x%08x\r\n", img_head.img_magic);
    debug("img_size:  0x%08x\r\n", img_head.img_size);
    debug("img_load:  0x%08x\r\n", img_head.img_load);
    debug("img_entry: 0x%08x\r\n", img_head.img_entry);

    app_entry = (void (*)(void))img_head.img_entry;
    dst_addr = (uint8_t *)img_head.img_load;

    count = img_head.img_size/2048;
    if (img_head.img_size%2048)
    {
        count++;
    }

    for (i = 0; i < count; i++)
    {
        spi_nand_read(&sunxi_spi0, (dst_addr+(2048*i)), (IMG_OFFSET_IN_FLASH+(2048*i)), 2048);
    }

    sunxi_spi_disable(&sunxi_spi0);
    dma_exit();

    arm32_mmu_disable();
    arm32_dcache_disable();
    arm32_icache_disable();
    arm32_interrupt_disable();

    app_entry();

    error("boot err.\r\n");

    while(1)
    {
    };
}