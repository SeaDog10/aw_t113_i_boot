#include "drv_iomux.h"

static unsigned int _port_base(unsigned int pin)
{
    unsigned int port = pin >> PIO_NUM_IO_BITS;

    switch (port) {
        case IO_PORTB:
            return 0x02000030;
            break;
        case IO_PORTC:
            return 0x02000060;
            break;
        case IO_PORTD:
            return 0x02000090;
            break;
        case IO_PORTE:
            return 0x020000c0;
            break;
        case IO_PORTF:
            return 0x020000f0;
            break;
        case IO_PORTG:
            return 0x02000120;
            break;
        default:
            return 0;
            break;
    }

    return 0;
}

static unsigned int _pin_num(unsigned int pin)
{
    return (pin & ((1 << PIO_NUM_IO_BITS) - 1));
}

int iomux_set_sel(struct iomux_cfg *iocfg)
{
    int ret = 0;
    unsigned int port_addr = 0;
    unsigned int pin_num   = 0;
    unsigned int addr = 0;
    unsigned int val = 0, v = 0;

    if (iocfg->port > IO_PORTG || iocfg->port < IO_PORTB)
    {
        ret = -1;
        goto _exit;
    }

    if (iocfg->pin > PIN_31 || iocfg->pin < PIN_0)
    {
        ret = -1;
        goto _exit;
    }

    if (iocfg->mux > IO_DISABLED || iocfg->mux < GPIO_INPUT)
    {
        ret = -1;
        goto _exit;
    }

    if (iocfg->pull > IO_PULL_RESERVE || iocfg->pull < IO_PULL_NONE)
    {
        ret = -1;
        goto _exit;
    }

    port_addr = _port_base(IO_PIN(iocfg->port, iocfg->pin));
    pin_num   = _pin_num(IO_PIN(iocfg->port, iocfg->pin));

    addr = port_addr + REG_GPIO_CFG0 + ((pin_num >> 3) << 2);
    val  = read32(addr);
    val &= ~(0xf << ((pin_num & 0x7) << 2));
    val |= ((iocfg->mux & 0xf) << ((pin_num & 0x7) << 2));
    write32(addr, val);

    switch (iocfg->pull)
    {
        case IO_PULL_NONE:
            v = 0x0;
            break;

        case IO_PULL_UP:
            v = 0x1;
            break;

        case IO_PULL_DOWN:
            v = 0x2;
            break;

        case IO_PULL_RESERVE:
            goto _exit;

        default:
            ret = -1;
            goto _exit;
    }

    addr = port_addr + REG_GPIO_PUL0 + ((pin_num >> 4) << 2);
    val     = read32(addr);
    val &= ~(v << ((pin_num & 0xf) << 1));
    val |= (v << ((pin_num & 0xf) << 1));
    write32(addr, val);

_exit:
    return ret;
}
