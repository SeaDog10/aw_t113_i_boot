#ifndef __DRV_IOMUX_H__
#define __DRV_IOMUX_H__

#include "board.h"

enum iomux_port
{
    IO_PORTB = 0,
    IO_PORTC = 1,
    IO_PORTD = 2,
    IO_PORTE = 3,
    IO_PORTF = 4,
    IO_PORTG = 5,
};

enum iomux_pin
{
    PIN_0  = 0,
    PIN_1  = 1,
    PIN_2  = 2,
    PIN_3  = 3,
    PIN_4  = 4,
    PIN_5  = 5,
    PIN_6  = 6,
    PIN_7  = 7,
    PIN_8  = 8,
    PIN_9  = 9,
    PIN_10 = 10,
    PIN_11 = 11,
    PIN_12 = 12,
    PIN_13 = 13,
    PIN_14 = 14,
    PIN_15 = 15,
    PIN_16 = 16,
    PIN_17 = 17,
    PIN_18 = 18,
    PIN_19 = 19,
    PIN_20 = 20,
    PIN_21 = 21,
    PIN_22 = 22,
    PIN_23 = 23,
    PIN_24 = 24,
    PIN_25 = 25,
    PIN_26 = 26,
    PIN_27 = 27,
    PIN_28 = 28,
    PIN_29 = 29,
    PIN_30 = 30,
    PIN_31 = 31,
};

enum iomux_sel
{
    GPIO_INPUT        = 0,
    GPIO_OUTPUT       = 1,
    IO_PERIPH_MUX2    = 2,
    IO_PERIPH_MUX3    = 3,
    IO_PERIPH_MUX4    = 4,
    IO_PERIPH_MUX5    = 5,
    IO_PERIPH_MUX6    = 6,
    IO_PERIPH_MUX7    = 7,
    IO_PERIPH_MUX8    = 8,
    IO_PERIPH_MUX14   = 14,
    IO_DISABLED       = 0xf,
};

enum iomux_pull
{
    IO_PULL_NONE    = 0x0,
    IO_PULL_UP      = 0x1,
    IO_PULL_DOWN    = 0x2,
    IO_PULL_RESERVE = 0x3,
};

#define PIO_NUM_IO_BITS 5
#define IO_PIN(x, y) (((unsigned int)(x << PIO_NUM_IO_BITS)) | y)

#define REG_GPIO_CFG0    0x00U
#define REG_GPIO_CFG1    0x04U
#define REG_GPIO_CFG2    0x08U
#define REG_GPIO_CFG3    0x0cU
#define REG_GPIO_DAT     0x10U
#define REG_GPIO_DRV0    0x14U
#define REG_GPIO_DRV1    0x18U
#define REG_GPIO_DRV2    0x1cU
#define REG_GPIO_DRV3    0x20U
#define REG_GPIO_PUL0    0x24U
#define REG_GPIO_PUL1    0x28U

struct iomux_cfg
{
    enum iomux_port port;
    enum iomux_pin  pin;
    enum iomux_sel  mux;
    enum iomux_pull pull;
};

int iomux_set_sel(struct iomux_cfg *iocfg);

#endif /* __DRV_IOMUX_H__ */
