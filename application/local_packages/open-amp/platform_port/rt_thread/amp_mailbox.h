#ifndef _AMP_MAILBOX_H_
#define _AMP_MAILBOX_H_

#ifdef __cplusplus
extern "C" {
#endif

// Include necessary headers
#include <rtdevice.h>
#include <rthw.h>
#include <rtthread.h>
#include <stdint.h>
// Define constants and macros
#define MAILBOX_SUCCESS 0
#define MAILBOX_ERROR   -1

typedef enum
{
    MBOX_CHANNEL_0 = 0,
    MBOX_CHANNEL_1 = 1,
    MBOX_CHANNEL_2 = 2,
    MBOX_CHANNEL_3 = 3,
    MBOX_CHANNEL_CNT,
} mbox_channel;

struct mbox_cmd_data {
    rt_uint32_t cmd;
    rt_uint32_t data;
};

typedef void (*mbox_callback_t)(int vector, void *param);

struct mbox_configure {
    rt_ubase_t           irq;
    rt_uint32_t          channel;
    mbox_callback_t      mbox_callback;
    void                *param;
    struct mbox_cmd_data init_msg;
};

struct rt_mbox_ops;

// Define data structures
struct rt_mbox_device {
    struct rt_device          mbox_dev;
    const struct rt_mbox_ops *ops;
    struct mbox_configure     config[MBOX_CHANNEL_CNT];
    struct rt_mutex           lock;
};

struct rt_mbox_ops {
    rt_err_t (*init)(struct rt_mbox_device *mbox, struct mbox_configure *cfg);
    rt_err_t (*configure)(struct rt_mbox_device *mbox, struct mbox_configure *cfg);
    rt_err_t (*control)(struct rt_mbox_device *mbox, int cmd, void *arg);
    rt_size_t (*sendmsg)(struct rt_mbox_device *mbox, rt_uint32_t channel, const void *buf, rt_size_t size);
    rt_size_t (*recvmsg)(struct rt_mbox_device *mbox, rt_uint32_t channel, void *buf, rt_size_t size);
};

// Function declarations
rt_err_t rt_hw_mbox_register(struct rt_mbox_device *mbox, const char *name, const struct rt_mbox_ops *ops, void *data);
void     rt_hw_mbox_isr(int irq, void *param);

#endif  // AMP_MAILBOX_H
