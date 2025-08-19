#include <drv_ov2640.h>
#include <rtdevice.h>
#include <drv_iomux.h>
#include <board.h>

#define DEV_ADDRESS      0x30 /* OV2640 address */

#define DBG_TAG "[drv.ov2640]"
#define BSP_ENBALE_OV2640_DEBUG 1

#define DBG_ENABLE
#if (BSP_ENBALE_OV2640_DEBUG == 1)
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_WARNING
#endif
#define DBG_COLOR
#include <rtdbg.h>

struct ov2640_device
{
    struct iomux_cfg ov_vsync;
    struct iomux_cfg ov_href;
    struct iomux_cfg ov_reset;
    struct iomux_cfg ov_pclk;
    struct iomux_cfg ov_pwdn;
    struct iomux_cfg ov_d0;
    struct iomux_cfg ov_d1;
    struct iomux_cfg ov_d2;
    struct iomux_cfg ov_d3;
    struct iomux_cfg ov_d4;
    struct iomux_cfg ov_d5;
    struct iomux_cfg ov_d6;
    struct iomux_cfg ov_d7;
    struct rt_i2c_bus_device *ov_i2c;
};

struct iomux_cfg rst_io =
{
    .port = IO_PORTE,
    .pin  = PIN_14,
    .mux  = GPIO_OUTPUT,
    .pull = IO_PULL_NONE,
};

struct iomux_cfg pwdn_io =
{
    .port = IO_PORTE,
    .pin  = PIN_15,
    .mux  = GPIO_OUTPUT,
    .pull = IO_PULL_NONE,
};

volatile rt_uint32_t jpeg_data_len   = 0;
volatile rt_uint8_t  jpeg_data_ok    = 0;
struct   rt_i2c_bus_device *i2c_bus  = RT_NULL;

static rt_err_t read_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msg[2];

    RT_ASSERT(bus != RT_NULL);

    msg[0].addr  = DEV_ADDRESS;
    msg[0].flags = RT_I2C_WR;
    msg[0].buf   = &reg;
    msg[0].len   = 2;

    msg[1].addr  = DEV_ADDRESS;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = len;
    msg[1].buf   = buf;

    if (rt_i2c_transfer(bus, msg, 2) == 2)
    {
        return RT_EOK;
    }

    return -RT_ERROR;
}

static rt_err_t write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t data)
{
    rt_uint8_t buf[2];
    struct rt_i2c_msg msgs;

    RT_ASSERT(bus != RT_NULL);

    buf[0] = reg ;
    buf[1] = data;

    msgs.addr = DEV_ADDRESS;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = 2;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }

    return -RT_ERROR;
}

static rt_err_t ov2640_read_id(struct rt_i2c_bus_device *bus)
{
    rt_uint8_t read_value[2];
    rt_uint16_t id = 0;
    read_reg(bus, OV2640_SENSOR_MIDH, 1, &read_value[0]);
    read_reg(bus, OV2640_SENSOR_MIDL, 1, &read_value[1]);
    id = ((rt_uint16_t)(read_value[0] << 8) & 0xFF00);
    id |= ((rt_uint16_t)(read_value[1]) & 0x00FF);

    if (id != OV2640_MID)
    {
        LOG_E("ov2640 init error, mid: 0x%04x", id);
        return -RT_ERROR;
    }

    LOG_I("ov2640 read mid success, mid: 0x%04x", id);

    read_reg(bus, OV2640_SENSOR_PIDH, 1, &read_value[0]);
    read_reg(bus, OV2640_SENSOR_PIDL, 1, &read_value[1]);
    id = ((rt_uint16_t)(read_value[0] << 8) & 0xFF00);
    id |= ((rt_uint16_t)(read_value[1]) & 0x00FF);

    if (id != OV2640_PID)
    {
        LOG_E("ov2640 init error, pid: 0x%04x", id);
        return -RT_ERROR;
    }

    LOG_I("ov2640 read hid success, pid: 0x%04x", id);

    return RT_EOK;
}

/* change ov2640 to jpeg mode */
void ov2640_jpeg_mode(void)
{
    rt_uint16_t i=0;
    /* set yun422 mode */
    for (i = 0; i < (sizeof(ov2640_yuv422_reg_tbl) / 2); i++)
    {
        write_reg(i2c_bus, ov2640_yuv422_reg_tbl[i][0],ov2640_yuv422_reg_tbl[i][1]);
    }

    /* set jpeg mode */
    for(i=0;i<(sizeof(ov2640_jpeg_reg_tbl)/2);i++)
    {
        write_reg(i2c_bus, ov2640_jpeg_reg_tbl[i][0],ov2640_jpeg_reg_tbl[i][1]);
    }
}

/* change ov2640 to rgb565 mode */
void ov2640_rgb565_mode(void)
{
    rt_uint16_t i=0;
    for (i = 0; i < (sizeof(ov2640_rgb565_reg_tbl) / 2); i++)
    {
        write_reg(i2c_bus, ov2640_rgb565_reg_tbl[i][0],ov2640_rgb565_reg_tbl[i][1]);
    }
}

/* set auto exposure */
void ov2640_set_auto_exposure(rt_uint8_t level)
{
    rt_uint8_t i = 0;
    rt_uint8_t *p = (rt_uint8_t*)OV2640_AUTOEXPOSURE_LEVEL[level];
    for (i = 0; i < 4; i++)
    {
        write_reg(i2c_bus, p[i*2],p[i*2+1]);
    }
}

/* set light mode
 * 0: auto
 * 1: sunny
 * 2: cloudy
 * 3: office
 * 4: home
 * */
void ov2640_set_light_mode(rt_uint8_t mode)
{
    rt_uint8_t regccval, regcdval, regceval;

    switch(mode)
    {
        case 0:
            write_reg(i2c_bus, 0xFF, 0x00);
            write_reg(i2c_bus, 0xC7, 0x10);
            return;

        case 2:
            regccval = 0x65;
            regcdval = 0x41;
            regceval = 0x4F;
            break;

        case 3:
            regccval = 0x52;
            regcdval = 0x41;
            regceval = 0x66;
            break;

        case 4:
            regccval = 0x42;
            regcdval = 0x3F;
            regceval = 0x71;
            break;

        default:
            regccval = 0x5E;
            regcdval = 0x41;
            regceval = 0x54;
            break;
    }

    write_reg(i2c_bus, 0xFF, 0x00);
    write_reg(i2c_bus, 0xC7, 0x40);
    write_reg(i2c_bus, 0xCC, regccval);
    write_reg(i2c_bus, 0xCD, regcdval);
    write_reg(i2c_bus, 0xCE, regceval);
}

/* set color saturation
 * 0: -2
 * 1: -1
 * 2: 0
 * 3: +1
 * 4: +2
 * */
void ov2640_set_color_saturation(rt_uint8_t sat)
{
    rt_uint8_t reg7dval = ((sat+2)<<4) | 0x08;
    write_reg(i2c_bus, 0xFF, 0X00);
    write_reg(i2c_bus, 0x7C, 0X00);
    write_reg(i2c_bus, 0x7D, 0X02);
    write_reg(i2c_bus, 0x7C, 0X03);
    write_reg(i2c_bus, 0x7D, reg7dval);
    write_reg(i2c_bus, 0x7D, reg7dval);
}

/* set brightness
 * 0: -2
 * 1: -1
 * 2: 0
 * 3: 1
 * 4: 2
 * */
void ov2640_set_brightness(rt_uint8_t bright)
{
  write_reg(i2c_bus, 0xff, 0x00);
  write_reg(i2c_bus, 0x7c, 0x00);
  write_reg(i2c_bus, 0x7d, 0x04);
  write_reg(i2c_bus, 0x7c, 0x09);
  write_reg(i2c_bus, 0x7d, bright << 4);
  write_reg(i2c_bus, 0x7d, 0x00);
}

/* set contrast
 * 0: -2
 * 1: -1
 * 2: 0
 * 3: 1
 * 4: 2
 * */
void ov2640_set_contrast(rt_uint8_t contrast)
{
    rt_uint8_t reg7d0val, reg7d1val;

    switch(contrast)
    {
        case 0:
            reg7d0val = 0x18;
            reg7d1val = 0x34;
            break;

        case 1:
            reg7d0val = 0x1C;
            reg7d1val = 0x2A;
            break;

        case 3:
            reg7d0val = 0x24;
            reg7d1val = 0x16;
            break;

        case 4:
            reg7d0val = 0x28;
            reg7d1val = 0x0C;
            break;

        default:
            reg7d0val = 0x20;
            reg7d1val = 0x20;
            break;
    }
    write_reg(i2c_bus, 0xff, 0x00);
    write_reg(i2c_bus, 0x7c, 0x00);
    write_reg(i2c_bus, 0x7d, 0x04);
    write_reg(i2c_bus, 0x7c, 0x07);
    write_reg(i2c_bus, 0x7d, 0x20);
    write_reg(i2c_bus, 0x7d, reg7d0val);
    write_reg(i2c_bus, 0x7d, reg7d1val);
    write_reg(i2c_bus, 0x7d, 0x06);
}

/* set special effects
 * 0: noraml
 * 1: negative film
 * 2: black-and-white
 * 3: the red
 * 4: the green
 * 5: the blue
 * 6: Retro
*/
void ov2640_set_special_effects(rt_uint8_t eft)
{
    rt_uint8_t reg7d0val = 0;
    rt_uint8_t reg7d1val = 0;
    rt_uint8_t reg7d2val = 0;

    switch(eft)
    {
        case 1:
            reg7d0val = 0x40;
            break;
        case 2:
            reg7d0val = 0x18;
            break;
        case 3:
            reg7d0val = 0x18;
            reg7d1val = 0x40;
            reg7d2val = 0xC0;
            break;
        case 4:
            reg7d0val = 0x18;
            reg7d1val = 0x40;
            reg7d2val = 0x40;
            break;
        case 5:
            reg7d0val = 0x18;
            reg7d1val = 0xA0;
            reg7d2val = 0x40;
            break;
        case 6:
            reg7d0val = 0x18;
            reg7d1val = 0x40;
            reg7d2val = 0xA6;
            break;
        default:
            reg7d0val = 0x00;
            reg7d1val = 0x80;
            reg7d2val = 0x80;
            break;
    }
    write_reg(i2c_bus, 0xff, 0x00);
    write_reg(i2c_bus, 0x7c, 0x00);
    write_reg(i2c_bus, 0x7d, reg7d0val);
    write_reg(i2c_bus, 0x7c, 0x05);
    write_reg(i2c_bus, 0x7d, reg7d1val);
    write_reg(i2c_bus, 0x7d, reg7d2val);
}

/* set the image output window */
void ov2640_set_window_size(rt_uint16_t sx,rt_uint16_t sy,rt_uint16_t width,rt_uint16_t height)
{
    rt_uint16_t endx;
    rt_uint16_t endy;
    rt_uint8_t temp;
    endx = sx + width / 2;
    endy = sy + height / 2;

    write_reg(i2c_bus, 0xFF, 0x01);
    read_reg(i2c_bus, 0x03, 1, &temp);
    temp &= 0xF0;
    temp |= ((endy & 0x03) << 2) | (sy & 0x03);
    write_reg(i2c_bus, 0x03, temp);
    write_reg(i2c_bus, 0x19, sy>>2);
    write_reg(i2c_bus, 0x1A, endy>>2);

    read_reg(i2c_bus, 0x32, 1, &temp);
    temp &= 0xC0;
    temp |= ((endx & 0x07) << 3) | (sx & 0x07);
    write_reg(i2c_bus, 0x32, temp);
    write_reg(i2c_bus, 0x17, sx>>3);
    write_reg(i2c_bus, 0x18, endx>>3);
}

/* set the image output size */
rt_uint8_t ov2640_set_image_out_size(rt_uint16_t width,rt_uint16_t height)
{
    rt_uint16_t outh, outw;
    rt_uint8_t temp;

    if(width%4)return 1;
    if(height%4)return 2;
    outw = width / 4;
    outh = height / 4;
    write_reg(i2c_bus, 0xFF, 0x00);
    write_reg(i2c_bus, 0xE0, 0x04);
    write_reg(i2c_bus, 0x5A, outw & 0XFF);
    write_reg(i2c_bus, 0x5B, outh & 0XFF);
    temp = (outw >> 8) & 0x03;
    temp |= (outh >> 6) & 0x04;
    write_reg(i2c_bus, 0x5C, temp);
    write_reg(i2c_bus, 0xE0, 0X00);

    return RT_EOK;
}

/* set the image window size */
rt_err_t ov2640_set_image_window_size(rt_uint16_t offx, rt_uint16_t offy, rt_uint16_t width, rt_uint16_t height)
{
    rt_uint16_t hsize, vsize;
    rt_uint8_t temp;
    if ((width % 4) || (height%4))
    {
        return -RT_ERROR;
    }
    hsize = width / 4;
    vsize = height / 4;
    write_reg(i2c_bus, 0XFF,0X00);
    write_reg(i2c_bus, 0XE0,0X04);
    write_reg(i2c_bus, 0X51,hsize&0XFF);
    write_reg(i2c_bus, 0X52,vsize&0XFF);
    write_reg(i2c_bus, 0X53,offx&0XFF);
    write_reg(i2c_bus, 0X54,offy&0XFF);
    temp=(vsize>>1)&0X80;
    temp|=(offy>>4)&0X70;
    temp|=(hsize>>5)&0X08;
    temp|=(offx>>8)&0X07;
    write_reg(i2c_bus, 0X55,temp);
    write_reg(i2c_bus, 0X57,(hsize>>2)&0X80);
    write_reg(i2c_bus, 0XE0,0X00);
    return RT_EOK;
}

/* set output resolution */
rt_uint8_t ov2640_set_image_size(rt_uint16_t width ,rt_uint16_t height)
{
   rt_uint8_t temp;
   write_reg(i2c_bus, 0xFF, 0x00);
   write_reg(i2c_bus, 0xE0, 0x04);
   write_reg(i2c_bus, 0xC0, (width >>3) & 0xFF);
   write_reg(i2c_bus, 0xC1, (height >> 3) & 0xFF);
   temp = (width & 0x07) << 3;
   temp |= height & 0x07;
   temp |= (width >> 4) & 0x80;
   write_reg(i2c_bus, 0x8C, temp);
   write_reg(i2c_bus, 0xE0, 0x00);

   return RT_EOK;
}

int rt_hw_ov2640_init(void)
{
    int ret = 0;
    rt_uint16_t i = 0;
    rt_uint16_t mid = 0;

    iomux_set_sel(&rst_io);
    iomux_set_sel(&pwdn_io);

    gpio_set_value(pwdn_io.port, pwdn_io.pin, 0);
    rt_thread_delay(20);

    gpio_set_value(rst_io.port, rst_io.pin, 0);
    rt_thread_delay(20);
    gpio_set_value(rst_io.port, rst_io.pin, 1);
    rt_thread_delay(20);

    i2c_bus = rt_i2c_bus_device_find("twi0");
    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("cannot find device twi0\n");
        return -RT_ERROR;
    }

    /* Prepare the camera to be configured */
    ret = write_reg(i2c_bus, OV2640_DSP_RA_DLMT, 0x01);
    if (ret != RT_EOK )
    {
        LOG_E("ov2640 write reg error!");
        return -RT_ERROR;
    }
    rt_thread_delay(10);
    ret = write_reg(i2c_bus, OV2640_SENSOR_COM7, 0x80);
    if (ret != RT_EOK)
    {
        LOG_E("ov2640 soft reset error!");
        return -RT_ERROR;
    }
    rt_thread_delay(20);

    ret = ov2640_read_id(i2c_bus);
    if (ret != RT_EOK )
    {
        LOG_E("ov2640 read id error!");
        return -RT_ERROR;
    }

    for (i = 0; i < sizeof(ov2640_svga_init_reg_tbl) / 2; i++)
    {
        write_reg(i2c_bus, ov2640_svga_init_reg_tbl[i][0], ov2640_svga_init_reg_tbl[i][1]);
    }

    ov2640_rgb565_mode();
    ov2640_set_light_mode(0);
    ov2640_set_color_saturation(3);
    ov2640_set_brightness(4);
    ov2640_set_contrast(3);
    ov2640_jpeg_mode();
    ov2640_set_image_window_size(0, 0, 320, 240);
    ov2640_set_image_out_size(320, 240);

    return RT_EOK;
}
MSH_CMD_EXPORT(rt_hw_ov2640_init, rt_hw_ov2640_init);
