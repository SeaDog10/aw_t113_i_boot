#ifndef _AMP_SIGIFACE_DEV_H_
#define _AMP_SIGIFACE_DEV_H_

#include "openamp_virtio_device.h"

#if defined(PKG_INTER_CORE_USING_HW_MAILBOX)
#define MBOX_RPMSG_MAGIC0 (0x524D5347U)
#define MBOX_RPMSG_MAGIC1 (0x5A5A5A50U)
#endif /* PKG_INTER_CORE_USING_HW_MAILBOX */

int amp_sigiface_dev_create(struct amp_sigiface_ops **sigiface_ops);

#endif
