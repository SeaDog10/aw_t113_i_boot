/*
 * Copyright (c) 2025, YC113
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_core.h"
#include "usb_hc_ehci.h"

#ifdef CONFIG_USB_EHCI_WITH_OHCI
#include "usb_hc_ohci.h"
#endif

#include "interrupt.h"

#if CONFIG_USBHOST_MAX_BUS != 2
#error "t113 has 2 usb host controller"
#endif

#if CONFIG_USB_OHCI_HCOR_OFFSET != 0x400
#error "t113 CONFIG_USB_OHCI_HCOR_OFFSET must be 0x400"
#endif

#if defined(CONFIG_USB_EHCI_HCOR_RESERVED_DISABLE)
#error "t113 usb ehci register need reserved"
#endif

#if !defined(CONFIG_USB_EHCI_CONFIGFLAG)
#error "t113 usb ehci has configflag register"
#endif

#if defined(CONFIG_USB_EHCI_ISO)
#error "t113 usb ehci no iso register"
#endif

void usb_clean_siddp(struct usbh_bus *bus)
{
    *(volatile rt_uint32_t *)(bus->hcd.reg_base + 0x810) &= ~(1 << 3);
}

void usb_hci_set_passby(struct usbh_bus *bus)
{
    /* AHB Master interface INCR16 enable */
    /* AHB Master interface INCR8 enable */
    /* AHB Master interface burst type INCR4 enable */
    /* AHB Master interface INCRX align enable */
    /* ULPI bypass enable */
    *(volatile rt_uint32_t *)(bus->hcd.reg_base + 0x800) |= (1 << 11) | (1 << 10) | (1 << 9) | (1 << 8) | (1 << 0);
}

void t113_ehci_isr(int vector, void *arg)
{
    rt_interrupt_enter();

    struct usbh_bus *bus = (struct usbh_bus *)arg;

    USB_LOG_DBG("t113_ehci_isr");

    extern void USBH_IRQHandler(uint8_t busid);
    USBH_IRQHandler(bus->hcd.hcd_id);

    rt_interrupt_leave();
}

#ifdef CONFIG_USB_EHCI_WITH_OHCI

void t113_ohci_isr(int vector, void *arg)
{
    rt_interrupt_enter();

    struct usbh_bus *bus = (struct usbh_bus *)arg;

    USB_LOG_DBG("t113_ohci_isr");

    extern void OHCI_IRQHandler(uint8_t busid);
    OHCI_IRQHandler(bus->hcd.hcd_id);

    rt_interrupt_leave();
}

#endif

void usb_hc_low_level_init(struct usbh_bus *bus)
{
    int vector;
    RT_ASSERT(bus->busid <= 1);

    usb_clean_siddp(bus);
    usb_hci_set_passby(bus);

    /* register EHCI interrupt callback */
    vector = 62 + (bus->busid > 0 ? 3 : 0);
    rt_hw_interrupt_install(vector, t113_ehci_isr, bus, RT_NULL);
    rt_hw_interrupt_umask(vector);

    /* register OHCI interrupt callback */
#ifdef CONFIG_USB_EHCI_WITH_OHCI
    rt_hw_interrupt_install(vector + 1, t113_ohci_isr, bus, RT_NULL);
    rt_hw_interrupt_set_priority(vector, 11 << 4);
    rt_hw_interrupt_umask(vector + 1);
#endif

    USB_LOG_DBG("usb%d vector : %d, phy : %X\n", bus->busid, vector, *(volatile rt_uint32_t *)(bus->hcd.reg_base + 0x810));
    USB_LOG_DBG("usb%d hc low level init success\n", bus->busid);
}

uint8_t usbh_get_port_speed(struct usbh_bus *bus, const uint8_t port)
{
    /* Defined by individual manufacturers */
    uint32_t regval;

    regval = EHCI_HCOR->portsc[port - 1];
    if ((regval & EHCI_PORTSC_LSTATUS_MASK) == EHCI_PORTSC_LSTATUS_KSTATE)
        return USB_SPEED_LOW;

    if (regval & EHCI_PORTSC_PE)
        return USB_SPEED_HIGH;
    else
        return USB_SPEED_FULL;
}

int __usbh_init(void)
{
#ifdef T113_USING_USB0_HOST
    /* USB0 MSC test OK */
    usbh_initialize(0, 0x04100000);
#endif

#ifdef T113_USING_USB1_HOST
    /* USB1 MSC test OK */
    usbh_initialize(1, 0x04200000);
#endif
    return 0;
}

#ifdef PKG_CHERRYUSB_HOST

#include <rtthread.h>
#include <rtdevice.h>

INIT_ENV_EXPORT(__usbh_init);

#endif
