/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include "rtthread.h"

/* ================ USB common Configuration ================ */

/* Protocol stack refresh cache enable*/
#define CONFIG_USB_DCACHE_ENABLE

/* DWC2 Control refresh cache enable*/
#define USB_DWC2_CACHE_ENABLE

#define CONFIG_USB_PRINTF(...)                  rt_kprintf(__VA_ARGS__)

#define usb_malloc(size)                        rt_malloc(size)
#define usb_free(ptr)                           rt_free(ptr)

/* 打开可屏蔽协议栈打印信息 */
#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE
#define CONFIG_USB_EHCI_DESC_DCACHE_ENABLE
#define CONFIG_USB_ALIGN_SIZE 32
/* data align size when use dma */
#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE                   32
#endif

#define CONFIG_USB_OHCI_HCOR_OFFSET 0x400
#define T113_USING_USB1_HOST
// #define T113_USING_USB0_HOST

/* attribute data into no cache ram */
// #define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))
#define USB_NOCACHE_RAM_SECTION

/* ================= USB Device Stack Configuration ================ */

/* Ep0 max transfer buffer, specially for receiving data from ep0 out */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN        256

/* Setup packet log for debug */
#define CONFIG_USBDEV_SETUP_LOG_PRINT

/* Check if the input descriptor is correct */
// #define CONFIG_USBDEV_DESC_CHECK

/* Enable test mode */
// #define CONFIG_USBDEV_TEST_MODE

//#define CONFIG_USBDEV_TX_THREAD
//#define CONFIG_USBDEV_RX_THREAD

#ifdef CONFIG_USBDEV_TX_THREAD
#ifndef CONFIG_USBDEV_TX_PRIO
#define CONFIG_USBDEV_TX_PRIO                   4
#endif
#ifndef CONFIG_USBDEV_TX_STACKSIZE
#define CONFIG_USBDEV_TX_STACKSIZE              2048
#endif
#endif

#ifdef CONFIG_USBDEV_RX_THREAD
#ifndef CONFIG_USBDEV_RX_PRIO
#define CONFIG_USBDEV_RX_PRIO                   4
#endif
#ifndef CONFIG_USBDEV_RX_STACKSIZE
#define CONFIG_USBDEV_RX_STACKSIZE              2048
#endif
#endif

#ifndef CONFIG_USBDEV_MSC_BLOCK_SIZE
#define CONFIG_USBDEV_MSC_BLOCK_SIZE            512
#endif

#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING   ""
#endif

#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
#define CONFIG_USBDEV_MSC_PRODUCT_STRING        ""
#endif

#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
#define CONFIG_USBDEV_MSC_VERSION_STRING        "0.01"
#endif

#ifndef CONFIG_USBDEV_AUDIO_VERSION
#define CONFIG_USBDEV_AUDIO_VERSION             0x0100
#endif

#ifndef CONFIG_USBDEV_AUDIO_MAX_CHANNEL
#define CONFIG_USBDEV_AUDIO_MAX_CHANNEL         8
#endif

#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
#define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE    128
#endif

#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
#define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE  1536
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
#define CONFIG_USBDEV_RNDIS_VENDOR_ID           0x0000ffff
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
#define CONFIG_USBDEV_RNDIS_VENDOR_DESC         "CherryUSB"
#endif

#define CONFIG_USBDEV_RNDIS_USING_LWIP

/* ================ USB HOST Stack Configuration ================== */

#define CONFIG_USBHOST_MAX_RHPORTS              1
#define CONFIG_USBHOST_MAX_EXTHUBS              2
#define CONFIG_USBHOST_MAX_EHPORTS              4
#define CONFIG_USBHOST_MAX_INTERFACES           8
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS     8
#define CONFIG_USBHOST_MAX_ENDPOINTS            4

#define CONFIG_USBHOST_MAX_CDC_ACM_CLASS        4
#define CONFIG_USBHOST_MAX_HID_CLASS            4
#define CONFIG_USBHOST_MAX_MSC_CLASS            2
#define CONFIG_USBHOST_MAX_AUDIO_CLASS          1
#define CONFIG_USBHOST_MAX_VIDEO_CLASS          1
#define CONFIG_USBHOST_MAX_RNDIS_CLASS          1

#define CONFIG_USBHOST_DEV_NAMELEN              16

#ifndef CONFIG_USBHOST_PSC_PRIO
#define CONFIG_USBHOST_PSC_PRIO                 4
#endif
#ifndef CONFIG_USBHOST_PSC_STACKSIZE
#define CONFIG_USBHOST_PSC_STACKSIZE            8192
#endif

/* 使能获取字符串描述符 */
#define CONFIG_USBHOST_GET_STRING_DESC

/* 协议栈中支持的最大字符串描述符数量 */
#define CONFIG_USBHOST_MAX_STRING_DESC_NUM      8

/* 字符串描述符的字符最大长度 */
#define CONFIG_USBHOST_MAX_STRING_SIZE          128

/* Ep0 max transfer buffer */
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN       512

#ifndef CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT
#define CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT 500
#endif

#ifndef CONFIG_USBHOST_MSC_TIMEOUT
#define CONFIG_USBHOST_MSC_TIMEOUT              5000
#endif


/* This parameter affects usb performance, and depends on (TCP_WND)tcp eceive windows size,
 * you can change to 2K ~ 16K and must be larger than TCP RX windows size in order to avoid being overflow.
 */
#ifndef CONFIG_USBHOST_RNDIS_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_RNDIS_ETH_MAX_RX_SIZE (2048)
#endif

/* Because lwip do not support multi pbuf at a time, so increasing this variable has no performance improvement */
#ifndef CONFIG_USBHOST_RNDIS_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_RNDIS_ETH_MAX_TX_SIZE (2048)
#endif

/* This parameter affects usb performance, and depends on (TCP_WND)tcp eceive windows size,
 * you can change to 2K ~ 16K and must be larger than TCP RX windows size in order to avoid being overflow.
 */
#ifndef CONFIG_USBHOST_CDC_NCM_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_CDC_NCM_ETH_MAX_RX_SIZE (2048)
#endif
/* Because lwip do not support multi pbuf at a time, so increasing this variable has no performance improvement */
#ifndef CONFIG_USBHOST_CDC_NCM_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_CDC_NCM_ETH_MAX_TX_SIZE (2048)
#endif

/* This parameter affects usb performance, and depends on (TCP_WND)tcp eceive windows size,
 * you can change to 2K ~ 16K and must be larger than TCP RX windows size in order to avoid being overflow.
 */
#ifndef CONFIG_USBHOST_ASIX_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_ASIX_ETH_MAX_RX_SIZE (8192)
#endif
/* Because lwip do not support multi pbuf at a time, so increasing this variable has no performance improvement */
#ifndef CONFIG_USBHOST_ASIX_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_ASIX_ETH_MAX_TX_SIZE (2048)
#endif

/* This parameter affects usb performance, and depends on (TCP_WND)tcp eceive windows size,
 * you can change to 2K ~ 16K and must be larger than TCP RX windows size in order to avoid being overflow.
 */
#ifndef CONFIG_USBHOST_RTL8152_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_RTL8152_ETH_MAX_RX_SIZE (2048)
#endif
/* Because lwip do not support multi pbuf at a time, so increasing this variable has no performance improvement */
#ifndef CONFIG_USBHOST_RTL8152_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_RTL8152_ETH_MAX_TX_SIZE (2048)
#endif

/* ================ USB Device Port Configuration ================*/

#define USBD_IRQHandler                         USBD_IRQHandler
#define USB_BASE                                (0xbfe00000UL)
#define USB_NUM_BIDIR_ENDPOINTS                 4

/* DWC2 Controller Uersing DMA */
// #define CONFIG_USB_DWC2_DMA_ENABLE


/* ================ USB Host Port Configuration ==================*/
#ifndef CONFIG_USBHOST_MAX_BUS
#define CONFIG_USBHOST_MAX_BUS 2
#endif

#ifndef CONFIG_USBHOST_PIPE_NUM
#define CONFIG_USBHOST_PIPE_NUM 10
#endif

#ifndef CONFIG_USB_EHCI_QTD_NUM
#define CONFIG_USB_EHCI_QTD_NUM (CONFIG_USBHOST_PIPE_NUM * 3)
#endif

#ifndef CONFIG_USB_EHCI_ITD_NUM
#define CONFIG_USB_EHCI_ITD_NUM 10
#endif

/* ================ EHCI Configuration ================ */

#define CONFIG_USB_EHCI_HCCR_OFFSET     (0x0)
#define CONFIG_USB_EHCI_FRAME_LIST_SIZE 1024
// #define CONFIG_USB_EHCI_WITH_OHCI

#define CONFIG_USB_EHCI_CONFIGFLAG
#define CONFIG_USB_EHCI_PORT_POWER

/* ================ USB Dcache Configuration ==================*/

#ifdef CONFIG_USB_DCACHE_ENABLE
/* style 1*/
void usb_dcache_clean(uintptr_t addr, uint32_t size);
void usb_dcache_invalidate(uintptr_t addr, uint32_t size);
void usb_dcache_flush(uintptr_t addr, uint32_t size);

/* style 2*/
// #define usb_dcache_clean(addr, size)
// #define usb_dcache_invalidate(addr, size)
// #define usb_dcache_flush(addr, size)
#endif

#ifndef usb_phyaddr2ramaddr
#define usb_phyaddr2ramaddr(addr) (addr)
#endif

#ifndef usb_ramaddr2phyaddr
#define usb_ramaddr2phyaddr(addr) (addr)
#endif

/* ================ CDC Class Configuration ==================*/
#ifndef CONFIG_USBHOST_MAX_ACM_SERIAL_CLASS
#define CONFIG_USBHOST_MAX_ACM_SERIAL_CLASS 4
#endif /* CONFIG_USBHOST_MAX_ACM_SERIAL_CLASS */

#ifndef CONFIG_USBHOST_MAX_ECM_NET_CLASS
#define CONFIG_USBHOST_MAX_ECM_NET_CLASS 2
#endif /* CONFIG_USBHOST_MAX_ECM_NET_CLASS */

#endif
