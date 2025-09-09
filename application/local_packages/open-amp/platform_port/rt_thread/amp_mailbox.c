#include "amp_mailbox.h"

#define DBG_TAG "amp mbox"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define MUTEX_TIMEOUT 1000
#define AMP_MBOX_DATA_FILTER

static rt_err_t rt_mbox_init(struct rt_device *dev)
{
    rt_err_t               result = RT_EOK;
    struct rt_mbox_device *mbox   = RT_NULL;

    RT_ASSERT(dev != RT_NULL);

    mbox = (struct rt_mbox_device *)dev;

    rt_memset(mbox->config, 0, (sizeof(struct mbox_configure) * MBOX_CHANNEL_CNT));

    return result;
}

static rt_err_t rt_mbox_open(struct rt_device *dev, rt_uint16_t oflag)
{
    struct rt_mbox_device *mbox = RT_NULL;
    RT_ASSERT(dev != RT_NULL);
    mbox = (struct rt_mbox_device *)dev;

    rt_memset(mbox->config, 0, (sizeof(struct mbox_configure) * MBOX_CHANNEL_CNT));

    dev->open_flag = oflag & 0xff;

    if(!(oflag & RT_DEVICE_FLAG_RDWR))
    {
        return (-RT_ERROR);
    }

    dev->open_flag |= RT_DEVICE_FLAG_RDWR;

    return RT_EOK;
}

static rt_ssize_t rt_mbox_read(struct rt_device *dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct rt_mbox_device *mbox   = RT_NULL;
    rt_err_t               result = (-RT_ERROR);

    RT_ASSERT(dev != RT_NULL);

    mbox = (struct rt_mbox_device *)dev;

    if(mbox->ops->recvmsg == RT_NULL)
    {
        LOG_E("mbox recvmsg func is null\n");
        return (-RT_ERROR);
    }

    if((dev->open_flag & RT_DEVICE_FLAG_RDWR) && (dev->ref_count > 0))
    {
        if(mbox->ops->control(mbox, RT_DEVICE_CTRL_GET_INT, (void *)(uintptr_t)pos) != 0)
        {
            result = mbox->ops->recvmsg(mbox, pos, buffer, size);
            mbox->ops->control(mbox, RT_DEVICE_CTRL_CLR_INT, (void *)(uintptr_t)pos);
        }
    }

    return result;
}

static rt_ssize_t rt_mbox_write(struct rt_device *dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct rt_mbox_device *mbox   = RT_NULL;
    rt_err_t               result = (-RT_ERROR);

    RT_ASSERT(dev != RT_NULL);

    mbox = (struct rt_mbox_device *)dev;

    if(mbox->ops->sendmsg == RT_NULL)
    {
        LOG_E("mbox sendmsg func is null\n");
        return (-RT_ERROR);
    }

    if((dev->open_flag & RT_DEVICE_FLAG_RDWR) && (dev->ref_count > 0))
    {
        result = rt_mutex_take(&(mbox->lock), MUTEX_TIMEOUT);
        if(result != RT_EOK)
        {
            return result;
        }

        result = mbox->ops->sendmsg(mbox, pos, buffer, size);

        rt_mutex_release(&(mbox->lock));
    }

    return result;
}

static rt_err_t rt_mbox_control(struct rt_device *dev, int cmd, void *args)
{
    struct rt_mbox_device *mbox = RT_NULL;

    RT_ASSERT(dev != RT_NULL);

    mbox = (struct rt_mbox_device *)dev;

    switch(cmd)
    {
        case RT_DEVICE_CTRL_CONFIG:
            if(args != RT_NULL)
            {
                struct mbox_configure *cfg = (struct mbox_configure *)args;

                if(cfg->channel >= MBOX_CHANNEL_CNT)
                {
                    rt_kprintf("mbox channel %d is invalid\n", cfg->channel);
                    return (-RT_ERROR);
                }

                if(mbox->config[cfg->channel].channel)
                {
                    LOG_E("client channel is already registered.");
                    return (-RT_ERROR);
                }

                mbox->config[cfg->channel].mbox_callback = cfg->mbox_callback;
                mbox->config[cfg->channel].param         = cfg->param;
                mbox->config[cfg->channel].channel       = cfg->channel;
#ifdef AMP_MBOX_DATA_FILTER
                mbox->config[cfg->channel].init_msg.cmd  = cfg->init_msg.cmd;
                mbox->config[cfg->channel].init_msg.data = cfg->init_msg.data;
#endif
                mbox->ops->configure(mbox, &mbox->config[cfg->channel]);
            }
            break;
        case RT_DEVICE_CTRL_CLOSE:
            if(args != RT_NULL)
            {
                struct mbox_configure *cfg = (struct mbox_configure *)args;
                rt_memset(&mbox->config[cfg->channel], 0, sizeof(struct mbox_configure));
                mbox->ops->control(mbox, cmd, (void *)(uintptr_t)cfg->channel);
            }
            break;
        case RT_DEVICE_CTRL_GET_INT:
            return mbox->ops->control(mbox, cmd, args);
            break;
        case RT_DEVICE_CTRL_CLR_INT:
            mbox->ops->control(mbox, cmd, args);
            break;

        default:
            break;
    }

    return RT_EOK;
}

static rt_err_t rt_mbox_close(struct rt_device *dev)
{
    struct rt_mbox_device *mbox   = RT_NULL;
    rt_uint8_t             i      = 0;
    rt_err_t               result = (-RT_ERROR);

    RT_ASSERT(dev != RT_NULL);

    mbox = (struct rt_mbox_device *)dev;

    rt_memset(mbox->config, 0, (sizeof(struct mbox_configure) * MBOX_CHANNEL_CNT));

    for(i = 0; i < MBOX_CHANNEL_CNT; i++)
    {
        result = mbox->ops->control(mbox, RT_DEVICE_CTRL_CLOSE, (void *)(uintptr_t)i);
    }

    dev->open_flag &= ~RT_DEVICE_FLAG_RDWR;

    return result;
}

/* ISR for mbox interrupt */
void rt_hw_mbox_isr(int irq, void *param)
{
    rt_uint32_t            i    = 0;
    struct mbox_cmd_data   msg  = {0};
    struct rt_mbox_device *mbox = RT_NULL;

    RT_ASSERT(param != RT_NULL);

    mbox = (struct rt_mbox_device *)param;

    for(i = 0; i < MBOX_CHANNEL_CNT; i++)
    {
        if(irq != mbox->config[i].irq)
        {
            continue;
        }

#ifdef AMP_MBOX_DATA_FILTER
        if(rt_mbox_read(&mbox->mbox_dev, i, (void *)&msg, sizeof(struct mbox_cmd_data)) != RT_EOK)
        {
            continue;
        }

        if(msg.cmd != mbox->config[i].init_msg.cmd)
        {
            LOG_E("mailbox cmd error! cmd: %X", msg.cmd);
            continue;
        }

        if(((msg.data & 0xF) != rt_hw_cpu_id()) || ((msg.data & ~0xF) != mbox->config[i].init_msg.data))
        {
            LOG_E("mailbox data error! data: %X\n", msg.data);
            continue;
        }
#endif
        if(mbox->config[i].mbox_callback)
        {
            LOG_D("openamp_hw_mbox_callback CPU#0, mbox channel: %d", i);
            mbox->config[i].mbox_callback(i, mbox->config[i].param);
        }
    }
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops mbox_device_ops = {rt_mbox_init, rt_mbox_open, rt_mbox_close, rt_mbox_read, rt_mbox_write, rt_mbox_control};
#endif

/*
 * mbox register
 */
rt_err_t rt_hw_mbox_register(struct rt_mbox_device *mbox, const char *name, const struct rt_mbox_ops *ops, void *data)
{
    struct rt_device *device = RT_NULL;
    RT_ASSERT(mbox != RT_NULL);

    device = &(mbox->mbox_dev);

    device->type        = RT_Device_Class_Bus;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
#ifdef RT_mbox_USING_HDR
    mbox->hdr = RT_NULL;
#endif

    rt_mutex_init(&(mbox->lock), "mbox", RT_IPC_FLAG_PRIO);

#ifdef RT_USING_DEVICE_OPS
    device->ops = &mbox_device_ops;
#else
    device->init    = rt_mbox_init;
    device->open    = rt_mbox_open;
    device->close   = rt_mbox_close;
    device->read    = rt_mbox_read;
    device->write   = rt_mbox_write;
    device->control = rt_mbox_control;
#endif
    mbox->ops = ops;

    device->user_data = data;

    /* register a character device */
    return rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);
}
