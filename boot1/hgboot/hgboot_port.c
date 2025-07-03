/***************************************************************************
 * Copyright (c) 2025 HGBOOT Authors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

#include "shell/shell.h"
#include "ymodem/ymodem.h"
#include "partition/partition.h"

static const char *hgboot_start_logo =
"                                                     \r\n\
  _    _  _____        ____   ____   ____ _______     \r\n\
 | |  | |/ ____|      |  _ \\ / __ \\ / __ \\__   __| \r\n\
 | |__| | |  __ ______| |_) | |  | | |  | | | |       \r\n\
 |  __  | | |_ |______|  _ <| |  | | |  | | | |       \r\n\
 | |  | | |__| |      | |_) | |__| | |__| | | |       \r\n\
 |_|  |_|\\_____|      |____/ \\____/ \\____/  |_|    \r\n\
 version:V1.0.0\r\n";

/* shell_putc_port: shell output single character interface, user should implement low-level output (e.g. UART send).*/
static void shell_putc_port(char ch)
{
    (void)ch;
    /* TODO: Implement low-level output, e.g. UART send ch */
    return;
}

/* shell_getc_port: shell input single character interface, user should implement low-level input (e.g. UART receive).*/
static char shell_getc_port(void)
{
    char ch = 0;
    /* TODO: Implement low-level input, e.g. UART receive to ch */
    return ch;
}

/* shell_port: shell port structure, binds input/output interfaces.*/
static shell_port_t shell_port =
{
    .shell_putchar = shell_putc_port,
    .shell_getchar = shell_getc_port,
};

/* ymodem_putchar_port: ymodem output single character interface, user should implement low-level output.*/
static void ymodem_putchar_port(char ch)
{
    (void)ch;
    /* TODO: Implement low-level output, e.g. UART send ch */
    return;
}

/* ymodem_getchar_port: ymodem input single character interface, user should implement low-level input, timeout is timeout parameter.*/
static int ymodem_getchar_port(char *ch, unsigned int timeout)
{
    char c = 0;
    /* TODO: Implement low-level input, e.g. UART receive to c, timeout is timeout */
    *(char *)ch = c;
    (void)timeout;
    return 0;
}

/* ymodem_port: ymodem port structure, binds input/output interfaces.*/
static ymdoem_port_t ymodem_port =
{
    .ymodem_putchar = ymodem_putchar_port,
    .ymodem_getchar = ymodem_getchar_port,
};

/* partition_init_port: Partition device initialization interface, user should implement low-level init.*/
static int partition_init_port(void)
{
    /* TODO: Implement low-level initialization */
    return 0;
}

/* partition_read_port: Partition device read interface, user should implement low-level read.*/
static int partition_read_port(unsigned int addr, unsigned char *buf, unsigned int size)
{
    /* TODO: Implement low-level read from addr to buf, size bytes */
    return 0;
}

/* partition_write_port: Partition device write interface, user should implement low-level write.*/
static int partition_write_port(unsigned int addr, unsigned char  *buf, unsigned int size)
{
    /* TODO: Implement low-level write from buf to addr, size bytes */
    return 0;
}

/* partition_erase_port: Partition device erase interface, user should implement low-level erase.*/
static int partition_erase_port(unsigned int addr, unsigned int size)
{
    /* TODO: Implement low-level erase from addr, size bytes */
    return 0;
}

/* partition_dev_ops: Partition device operation structure, binds init/read/write/erase interfaces.*/
static partition_dev_ops_t partition_dev_ops =
{
    .init =  partition_init_port,
    .read =  partition_read_port,
    .write = partition_write_port,
    .erase = partition_erase_port,
};

int hgboot_init(void)
{
    int ret = 0;
    char *dev_name = "nand_flash";

    /* Initialize shell component */
    ret = shell_init(&shell_port, hgboot_start_logo);
    if (ret != 0)
    {
        return ret;
    }

    /* Initialize ymodem component */
    ret = ymodem_init(&ymodem_port);
    if (ret != 0)
    {
        return ret;
    }

    /* Register partition device */
    ret  = partition_device_register(dev_name, &partition_dev_ops, 7*1024*1024);
    if (ret != 0)
    {
        return ret;
    }

    /* Register partitions: APP1, APP2, Download, Param */
    ret += partition_register("APP1",     dev_name, 0 * 1024 * 1024, 1 * 1024 * 1024); /* 1M for firmware 1         */
    ret += partition_register("APP2",     dev_name, 1 * 1024 * 1024, 1 * 1024 * 1024); /* 1M for firmware 2         */
    ret += partition_register("Download", dev_name, 2 * 1024 * 1024, 1 * 1024 * 1024); /* 1M for firmware download  */
    ret += partition_register("Param",    dev_name, 3 * 1024 * 1024, 64 * 1024);       /* 64K for ota parameters    */

    if (ret != 0)
    {
        return ret;
    }

    return 0;
}
