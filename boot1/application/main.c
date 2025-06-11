#include "interrupt.h"
#include "cp15.h"
#include "drv_uart.h"
#include "drv_iomux.h"
#include "drv_clk.h"
#include "drv_spi.h"
#include "drv_nand.h"
#include "memheap.h"
#include "shell.h"

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

struct spi_handle spi1 = {
    .base = SPI1_BASE_ADDR,
    .irq  = SPI1_IRQ,
    .id   = SPI1,
    .cfg  =
    {
        .clk_rate = 10000000,
        .mode     = 0,
    },
    .gpio_cs =
    {
        .port = IO_PORTD,
        .pin  = PIN_10,
        .mux  = IO_PERIPH_MUX4,
        .pull = IO_PULL_RESERVE,
    },
    .gpio_sck =
    {
        .port = IO_PORTD,
        .pin  = PIN_11,
        .mux  = IO_PERIPH_MUX4,
        .pull = IO_PULL_RESERVE,
    },
    .gpio_mosi =
    {
        .port = IO_PORTD,
        .pin  = PIN_12,
        .mux  = IO_PERIPH_MUX4,
        .pull = IO_PULL_RESERVE,
    },
    .gpio_miso =
    {
        .port = IO_PORTD,
        .pin  = PIN_13,
        .mux  = IO_PERIPH_MUX4,
        .pull = IO_PULL_RESERVE,
    },
    .gpio_wp =
    {
        .port = IO_PORTD,
        .pin  = PIN_15,
        .mux  = IO_PERIPH_MUX4,
        .pull = IO_PULL_UP,
    },
    .gpio_hold =
    {
        .port = IO_PORTD,
        .pin  = PIN_14,
        .mux  = IO_PERIPH_MUX4,
        .pull = IO_PULL_UP,
    }
};

const char *start_logo =
"                                                     \r\n\
  _    _  _____        ____   ____   ____ _______     \r\n\
 | |  | |/ ____|      |  _ \\ / __ \\ / __ \\__   __| \r\n\
 | |__| | |  __ ______| |_) | |  | | |  | | | |       \r\n\
 |  __  | | |_ |______|  _ <| |  | | |  | | | |       \r\n\
 | |  | | |__| |      | |_) | |__| | |__| | | |       \r\n\
 |_|  |_|\\_____|      |____/ \\____/ \\____/  |_|    \r\n\
 version:V0.0.1\r\n";

static void shell_uart_putc(void *arg, char c)
{
    struct uart_handle *uart = (struct uart_handle *)arg;

    uart_putc(uart, c);
}

static char shell_uart_getc(void *arg)
{
    struct uart_handle *uart = (struct uart_handle *)arg;

    return uart_getc(uart);
}

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

static int spi1_test(int argc, char **argv)
{
    int i = 0;
    int ret = 0;
    unsigned char tx_buf1[4] = {0};
    unsigned char tx_buf2[4] = {0};
    struct spi_trans_msg msg1 = {0};
    struct spi_trans_msg msg2 = {0};

    tx_buf1[0] = 0x0f;
    tx_buf1[1] = 0x00;
    tx_buf1[2] = 0x00;
    tx_buf1[3] = 0x00;

    msg1.tx_buf = &tx_buf1[0];
    msg1.tx_len = 4;
    msg1.rx_buf = U_NULL;
    msg1.rx_len = 0;
    msg1.dummylen = 0;

    tx_buf2[0] = 1;
    tx_buf2[1] = 2;
    tx_buf2[2] = 3;
    tx_buf2[3] = 4;

    msg2.tx_buf = &tx_buf2[0];
    msg2.tx_len = 4;
    msg2.rx_buf = U_NULL;
    msg2.rx_len = 0;
    msg2.dummylen = 0;

    ret = spi_transfer_then_transfer(&spi1, &msg1, &msg2);
    // ret = spi_transfer(&spi1, &msg2);
    if (ret != 0)
    {
        s_printf("spi1 transfer failed\r\n");
    }
    else
    {
        s_printf("spi1 transfer success\r\n");
    }

    return 0;
}

static struct shell_command spi1_test_cmd =
{
    .name = "spi1_test",
    .desc = "spi1 test",
    .func = spi1_test,
    .next = SHELL_NULL,
};

unsigned char read_buffer[2048] = {0};
unsigned char write_buffer[2048] = {0};

#define TEST_PAGE 2048

static void dump_page(void *buffer, unsigned int len)
{
    unsigned int start_base = buffer;
    unsigned int i = 0, j=  0;
    unsigned char val = 0;
    char ascii_buf[17] = {0};

    s_printf("Dump 0x%p %dBytes\r\n", start_base, len);
    s_printf("Offset      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");

    for (i = 0; i < len; i += 16)
    {
        s_printf("0x%08X  ", start_base + i);

        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                val = *((unsigned char *)(start_base + i + j));
                s_printf("%02X ", val);
                ascii_buf[j] = (val >= 32 && val <= 126) ? val : '.';
            }
            else
            {
                s_printf("   ");
                ascii_buf[j] = ' ';
            }
        }

        s_printf(" %s\r\n", ascii_buf);
    }
}

static int nand_test(int argc, char **argv)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < 2048; i++)
    {
        write_buffer[i] = i;
        read_buffer[i] = 0;
    }

    ret = nand_page_read(&nand, TEST_PAGE, read_buffer);
    if (ret != 0)
    {
        s_printf("nand page read failed\r\n");
    }
    else
    {
        s_printf("nand page read success\r\n");
        s_printf("read_buffer : \r\n");
        dump_page((void *)read_buffer, 2048);
    }

    ret = nand_erase_page(&nand, TEST_PAGE);
    if (ret != 0)
    {
        s_printf("nand page erase failed\r\n");
    }
    else
    {
        s_printf("nand page erase success\r\n");
    }

    ret = nand_page_read(&nand, TEST_PAGE, read_buffer);
    if (ret != 0)
    {
        s_printf("nand page read failed\r\n");
    }
    else
    {
        s_printf("nand page read success\r\n");
        s_printf("read_buffer : \r\n");
        dump_page((void *)read_buffer, 2048);
    }

    ret = nand_page_write(&nand, TEST_PAGE, write_buffer);
    if (ret != 0)
    {
        s_printf("nand page write failed\r\n");
    }
    else
    {
        s_printf("nand page write success\r\n");
    }

    ret = nand_page_read(&nand, TEST_PAGE, read_buffer);
    if (ret != 0)
    {
        s_printf("nand page read failed\r\n");
    }
    else
    {
        s_printf("nand page read success\r\n");
        s_printf("read_buffer : \r\n");
        dump_page((void *)read_buffer, 2048);
    }

    return 0;
}

static struct shell_command nand_cmd =
{
    .name = "nand_tset",
    .desc = "test nand function",
    .func = nand_test,
    .next = SHELL_NULL,
};

int main(void)
{
    int ret = 0;

    interrupt_disable();
    interrupt_init();
    interrupt_enable();

    drv_clk_init();

    uart_init(&uart0);

    shell_init(shell_uart_putc, &uart0, shell_uart_getc, &uart0, start_logo);

    ret = rt_memheap_init(&system_heap, HEAP_BEGIN, HEAP_SIZE);
    if (ret != 0)
    {
        s_printf("memheap init failed\r\n");
    }

    ret = spi_init(&spi1);
    if (ret != 0)
    {
        s_printf("spi1 init failed\r\n");
    }

    ret = nand_init(&nand);
    if (ret != 0)
    {
        s_printf("nand init failed\r\n");
    }
    else
    {
        s_printf("nand init success\r\n");
        s_printf("nand info : \r\n");
        s_printf("mfr_id          = 0x%02x\r\n", nand.info.id.mfr_id);
        s_printf("dev_id          = 0x%04x\r\n", nand.info.id.dev_id);
        s_printf("page_size       = %d\r\n", nand.info.page_size);
        s_printf("spare_size      = %d\r\n", nand.info.spare_size);
        s_printf("pages_per_block = %d\r\n", nand.info.pages_per_block);
        s_printf("blocks_total    = %d\r\n", nand.info.blocks_total);
    }

    shell_register_command(&free_cmd);
    shell_register_command(&clk_cmd);
    shell_register_command(&nand_cmd);
    shell_register_command(&spi1_test_cmd);

    while(1)
    {
        shell_servise();
    }
}


