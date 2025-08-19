#ifndef __DRV_TWI_H__
#define __DRV_TWI_H__

#include <rtthread.h>

#define TWI0_BASE_ADDR          (0x02502000)
#define TWI1_BASE_ADDR          (0x02502400)
#define TWI2_BASE_ADDR          (0x02502800)
#define TWI3_BASE_ADDR          (0x02502C00)

#define TWI0_IRQ_NUM            (41)
#define TWI1_IRQ_NUM            (42)
#define TWI2_IRQ_NUM            (43)
#define TWI3_IRQ_NUM            (44)

#define REG_TWI_ADDR            0x00
#define REG_TWI_XADDR           0x04
#define REG_TWI_DATA            0x08
#define REG_TWI_CNTR            0x0C
#define REG_TWI_STAT            0x10
#define REG_TWI_CCR             0x14
#define REG_TWI_SRST            0x18
#define REG_TWI_EFR             0x1C
#define REG_TWI_LCR             0x20

#define TWI_CCR             0x0014
#define TWI_CNTR            0x000C
#define TWI_DATA            0x0008
#define TWI_STAT            0x0010
#define TWI_EFR             0x001C
#define TWI_SRST            0x0018

#define I2C_RATE_100K       ((3 << 0) | (11 << 3))

#define ENABLE_INT          (1 << 7)
#define DISABLE_INT         (~(1 << 7))
#define INT_FLAG            (1 << 3)
#define ENABLE_BUS          (1 << 6)

#define START_SIGNAL        (1 << 5)
#define STOP_SIGNAL         (1 << 4)

#define ACK_SIGNAL          (1 << 2)
#define NACK_SIGNAL         (~(1 << 2))

#define I2C_SRST            (1)

#define I2C_TIMEOUT         100

#define TIMEOUT(timems, ...)                    \
({                                              \
    rt_int32_t ret = 0;                         \
    rt_uint32_t cnt = 0;                        \
    while (1)                                   \
    {                                           \
        ret = (__VA_ARGS__);                    \
        if (ret != 0)                           \
        {                                       \
            if (cnt > timems)                   \
            {                                   \
                ret = 1;                        \
                break;                          \
            }                                   \
            cnt++;                              \
            rt_thread_mdelay(1);               	\
        }                                       \
        else                                    \
        {                                       \
            ret = 0;                            \
            break;                              \
        }                                       \
    }                                           \
    ret;                                        \
})

int rt_hw_twi_init(void);

#endif
