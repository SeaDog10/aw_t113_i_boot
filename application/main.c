#include "main.h"
#include "fdt.h"
#include "ff.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_clk.h"
#include "sunxi_dma.h"
#include "sdmmc.h"
#include "arm32.h"
#include "reg-ccu.h"
#include "debug.h"
#include "board.h"
#include "barrier.h"

int main(void)
{
    gpio_t led = GPIO_PIN(PORTC, 0);

    board_init();
    sunxi_clk_init();
    sunxi_dram_init();

    sunxi_clk_dump();

    debug("sdhci: init\r\n");
    if (sunxi_sdhci_init(&sdhci0) != 0)
    {
        fatal("SMHC: %s controller init failed\r\n", sdhci0.name);
    }
    else
    {
        info("SMHC: %s controller v%" PRIx32 " initialized\r\n", sdhci0.name, sdhci0.reg->vers);
    }

    debug("SPI: init\r\n");
    if (sunxi_spi_init(&sunxi_spi0) != 0)
    {
        fatal("SPI: init failed\r\n");
    }

    while(1)
    {
        debug("Hello hg'boot\r\n");
        sunxi_gpio_set_value(led, 0);
        mdelay(500);
        sunxi_gpio_set_value(led, 1);
        mdelay(500);
    }
}