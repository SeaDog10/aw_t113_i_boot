#include "interrupt.h"
#include "cp15.h"
#include "board.h"
#include "drv_uart.h"
#include "drv_iomux.h"
#include "drv_clk.h"
#include "shell_port.h"
#include "ymodem_port.h"
#include "littlefs_port.h"
#include "lfs.h"
#include "partition_port.h"
#include "partition.h"
#include "memheap.h"
#include "shell.h"
#include "ymodem.h"

struct uart_handle uart0 = {
    .base     = UART0_BASE_ADDR,
    .irq      = UART0_IRQ,
    .id       = UART0,
    .cfg      =
    {
       .baud_rate = BAUD_RATE_115200,
       .data_bits = UART_DATA_BITS_8,
       .stop_bits = UART_STOP_BITS_1,
       .parity    = UART_PARITY_NONE,
    },
    .gpio_tx =
    {
        .port = IO_PORTG,
        .pin  = PIN_17,
        .mux  = IO_PERIPH_MUX7,
        .pull = IO_PULL_RESERVE,
    },
    .gpio_rx =
    {
        .port = IO_PORTG,
        .pin  = PIN_18,
        .mux  = IO_PERIPH_MUX7,
        .pull = IO_PULL_RESERVE,
    },
};

struct spi_nand_handle nand =
{
    .nand_spi =
    {
        .base = SPI0_BASE_ADDR,
        .irq  = SPI0_IRQ,
        .id   = SPI0,
        .cfg  =
        {
            .clk_rate = 100000000,
            .mode     = 0,
        },
        .gpio_cs =
        {
            .port = IO_PORTC,
            .pin  = PIN_3,
            .mux  = IO_PERIPH_MUX2,
            .pull = IO_PULL_RESERVE,
        },
        .gpio_sck =
        {
            .port = IO_PORTC,
            .pin  = PIN_2,
            .mux  = IO_PERIPH_MUX2,
            .pull = IO_PULL_RESERVE,
        },
        .gpio_mosi =
        {
            .port = IO_PORTC,
            .pin  = PIN_4,
            .mux  = IO_PERIPH_MUX2,
            .pull = IO_PULL_RESERVE,
        },
        .gpio_miso =
        {
            .port = IO_PORTC,
            .pin  = PIN_5,
            .mux  = IO_PERIPH_MUX2,
            .pull = IO_PULL_RESERVE,
        },
        .gpio_wp =
        {
            .port = IO_PORTC,
            .pin  = PIN_6,
            .mux  = IO_PERIPH_MUX2,
            .pull = IO_PULL_UP,
        },
        .gpio_hold =
        {
            .port = IO_PORTC,
            .pin  = PIN_7,
            .mux  = IO_PERIPH_MUX2,
            .pull = IO_PULL_UP,
        }
    },
};

static int memfree(int argc, char **argv)
{
    unsigned int total   = 0;
    unsigned int used    = 0;
    unsigned int maxused = 0;

    rt_memheap_info(&system_heap, &total, &used, &maxused);

    s_printf("memheap info:\r\n");
    s_printf("__memheap_start : 0x%08x\r\n__memheap_end   : 0x%08x\r\n", (unsigned int)HEAP_BEGIN, (unsigned int)HEAP_END);
    s_printf("total  : 0x%08x\r\nused   : 0x%08x\r\nmaxused: 0x%08x\r\n", total, used, maxused);

    return 0;
}

static struct shell_command free_cmd =
{
    .name = "memfree",
    .desc = "show memheap info",
    .func = memfree,
    .next = SHELL_NULL,
};

static int clkdump(int argc, char **argv)
{
    unsigned int pll_cpu = 0;
    unsigned int pll_peri_1x = 0;
    unsigned int pll_peri_2x = 0;
    unsigned int pll_peri_800M = 0;
    unsigned int ahbs_clk = 0;
    unsigned int apb0_clk = 0;
    unsigned int apb1_clk = 0;

    pll_cpu = drv_clk_get_pll_cpu();
    pll_peri_1x = drv_clk_get_pll_peri_1x();
    pll_peri_2x = drv_clk_get_pll_peri_2x();
    pll_peri_800M = drv_clk_get_pll_peri_800M();
    ahbs_clk = drv_clk_get_ahbs_clk();
    apb0_clk = drv_clk_get_apb0_clk();
    apb1_clk = drv_clk_get_apb1_clk();

    s_printf("clk info:\r\n");
    s_printf("PLL_CPU        : %dHz\r\n", pll_cpu);
    s_printf("PLL_PERI(1X)   : %dHz\r\n", pll_peri_1x);
    s_printf("PLL_PERI(2X)   : %dHz\r\n", pll_peri_2x);
    s_printf("PLL_PERI(800M) : %dHz\r\n", pll_peri_800M);
    s_printf("AHBS_CLK       : %dHz\r\n", ahbs_clk);
    s_printf("APB0_CLK       : %dHz\r\n", apb0_clk);
    s_printf("APB1_CLK       : %dHz\r\n", apb1_clk);

    return 0;
}

static struct shell_command clk_cmd =
{
    .name = "clkdump",
    .desc = "show clk info",
    .func = clkdump,
    .next = SHELL_NULL,
};

static lfs_file_t file;

static int lfs_test(int argc, char **argv)
{
    int ret = 0;
    char boot_count = 0;

    ret = lfs_file_open(&nand_lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    if (ret != 0)
    {
        s_printf("lfs file open failed\r\n");
        return -1;
    }
    else
    {
        s_printf("lfs file open success\r\n");
    }

    ret = lfs_file_read(&nand_lfs, &file, &boot_count, 1);
    s_printf("lfs file read %d\r\n", ret);

    boot_count++;

    ret = lfs_file_rewind(&nand_lfs, &file);
    if (ret != 0)
    {
        s_printf("lfs file rewind failed\r\n");
        return -1;
    }
    else
    {
        s_printf("lfs file rewind success\r\n");
    }

    ret = lfs_file_write(&nand_lfs, &file, &boot_count, 1);
    s_printf("lfs file write %d\r\n", ret);

    ret = lfs_file_close(&nand_lfs, &file);
    if (ret != 0)
    {
        s_printf("lfs file close failed\r\n");
        return -1;
    }
    else
    {
        s_printf("lfs file close success\r\n");
    }

    s_printf("boot_count: %d\n", boot_count);

    return 0;
}
static struct shell_command lfs_cmd =
{
    .name = "lfs_tset",
    .desc = "test lfs function",
    .func = lfs_test,
    .next = SHELL_NULL,
};

int main(void)
{
    int ret = 0;

    interrupt_disable();
    interrupt_init();
    interrupt_enable();

    drv_clk_init();

    (void)shell_register();

    ret = ymodem_register();
    if (ret != 0)
    {
        s_printf("ymodem init err.\r\n");
    }

    ret = rt_memheap_init(&system_heap, HEAP_BEGIN, HEAP_SIZE);
    if (ret != 0)
    {
        s_printf("memheap init failed\r\n");
    }

    ret = nand_init(&nand);
    if (ret != 0)
    {
        s_printf("nand init failed\r\n");
    }

    ret = partition_nand_register();
    if (ret != 0)
    {
        s_printf("partition nand register failed\r\n");
    }

    ret = drv_lfs_mount();
    if (ret != 0)
    {
        s_printf("lfs mount failed %d\r\n", ret);
    }

    shell_register_command(&free_cmd);
    shell_register_command(&clk_cmd);
    shell_register_command(&lfs_cmd);

    while(1)
    {
        shell_servise();
    }
}


