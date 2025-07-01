#include "ymodem.h"

#define YMODEM_NULL     0x00
#define YMODEM_SOH      0x01
#define YMODEM_STX      0x02
#define YMODEM_EOT      0x04
#define YMODEM_ACK      0x06
#define YMODEM_NAK      0x15
#define YMODEM_CAN      0x18
#define YMODEM_C        0x43

#define PACKET_SIZE_128 (128)

#ifdef SUPPORT_1K
#define PACKET_SIZE_1K  (1024)
#endif

struct ymodem_frame
{
    char head;
    int  pkg_size;
    char pack_num;
    char pack_mask;
    char *buf;
    int  offset;
    char crc_h;
    char crc_l;
    int file_size;
};

ymodem_callback_t *y_cb = YMODEM_NULL;
ymdoem_port_t *y_port   = YMODEM_NULL;

static ymodem_head_t y_head  = {0};
static ymodem_data_t y_data  = {0};

static struct ymodem_frame cache_frame     = {0};

#ifdef SUPPORT_1K
static char   buffer_cache[PACKET_SIZE_1K]   = {0};
#else
static char   buffer_cache[PACKET_SIZE_128]   = {0};
#endif

static unsigned short ymodem_crc16(char *buf, unsigned short len)
{
    unsigned short crc = 0;
    unsigned short i   = 0;
    unsigned char  j   = 0;

    for (i = 0; i < len; ++i)
    {
        crc ^= ((unsigned short)buf[i] << 8);
        for (j = 0; j < 8; ++j)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static int ymodem_strtol(const char *str, int n, int *out)
{
    int result = 0;
    int sign = 1;
    int i = 0;
    int started = 0;

    if (!str || n <= 0 || !out)
    {
        return -1;
    }

    while (i < n && (str[i] == ' ' || str[i] == '\t'))
    {
        i++;
    }

    if (i < n && str[i] == '-')
    {
        sign = -1;
        i++;
    }
    else if (i < n && str[i] == '+')
    {
        i++;
    }

    for (; i < n; i++)
    {
        char c = str[i];

        if (c >= '0' && c <= '9')
        {
            result = result * 10 + (c - '0');
            started = 1;
        }
        else
        {
            break;
        }
    }

    if (!started)
    {
        return -2;
    }

    *out = result * sign;

    return 0;
}

int ymodem_init(ymdoem_port_t *port)
{
    if (port == YMODEM_NULL)
    {
        return YMODEM_ERR_PARAM;
    }

    y_port = port;

    return YMODEM_OK;
}

static int ymodem_receive_packet(struct ymodem_frame *frame, int retries)
{
    int size = 0;
    unsigned short recv_crc = 0;
    unsigned short calc_crc = 0;

    while (retries > 0)
    {
        if (y_port->ymodem_getchar(&frame->head, 10) != 0)
        {
            retries--;
            continue;
        }

        if (frame->head == YMODEM_SOH)
        {
            frame->pkg_size = PACKET_SIZE_128;
        }
#ifdef SUPPORT_1K
        else if (frame->head == YMODEM_STX)
        {
            frame->pkg_size = PACKET_SIZE_1K;
        }
#endif
        else if (frame->head == YMODEM_EOT)
        {
            return YMODEM_EOT;
        }
        else
        {
            retries--;
            continue;
        }

        if(y_port->ymodem_getchar(&frame->pack_num, 10) != 0)
        {
            retries--;
            y_port->ymodem_putchar(YMODEM_NAK);
            continue;
        }

        if(y_port->ymodem_getchar(&frame->pack_mask, 10) != 0)
        {
            retries--;
            y_port->ymodem_putchar(YMODEM_NAK);
            continue;
        }

        if ((frame->pack_num ^ frame->pack_mask) != 0xFF)
        {
            retries--;
            y_port->ymodem_putchar(YMODEM_NAK);
            continue;
        }

        frame->offset = 0;
        size = frame->pkg_size;

        while (size > 0)
        {
            if (y_port->ymodem_getchar(&frame->buf[frame->offset], 10) == 0)
            {
                frame->offset++;
                size--;
            }
        }

        if(y_port->ymodem_getchar(&frame->crc_h, 10) != 0)
        {
            retries--;
            y_port->ymodem_putchar(YMODEM_NAK);
            continue;
        }

        if(y_port->ymodem_getchar(&frame->crc_l, 10) != 0)
        {
            retries--;
            y_port->ymodem_putchar(YMODEM_NAK);
            continue;
        }

        recv_crc = ((frame->crc_h << 8) & 0xFF00) | (frame->crc_l & 0x00FF);
        calc_crc = ymodem_crc16(frame->buf, frame->pkg_size);

        if (recv_crc != calc_crc)
        {
            retries--;
            y_port->ymodem_putchar(YMODEM_NAK);
            continue;
        }

        return YMODEM_OK;
    }

    return YMODEM_ERR_TIMEOUT;
}

int ymodem_receive(ymodem_callback_t *callback)
{
    int begin_recv = 0;
    int i = 0;
    int ret = 0;
    int fname_len = 0;
    int fsize_len = 0;
    int end_err = 0 ;

    if (callback == YMODEM_NULL || y_port == YMODEM_NULL)
    {
        return YMODEM_ERR_PARAM;
    }

    y_cb = callback;
    cache_frame.buf = buffer_cache;

    for (i = 0; i < MAX_RETRY; i++)
    {
        y_port->ymodem_putchar(YMODEM_C);
        if (ymodem_receive_packet(&cache_frame, MAX_RETRY) == 0)
        {
            begin_recv = 1;
            break;
        }
    }

    if (begin_recv == 0)
    {
        if (y_cb->ymodem_abort)
        {
            y_cb->ymodem_abort(YMODEM_ERR_TIMEOUT);
        }
        return YMODEM_ERR_TIMEOUT;
    }

    for (i = 0; i < cache_frame.pkg_size; i++)
    {
        if (cache_frame.buf[i] == 0x00)
        {
            fname_len = i + 1;
            break;
        }
    }

    for (i = fname_len; i < cache_frame.pkg_size; i++)
    {
        if (cache_frame.buf[i] == ' ')
        {
            fsize_len = i - fname_len;
            break;
        }
    }

    ret = ymodem_strtol(&cache_frame.buf[fname_len], fsize_len, &cache_frame.file_size);
    if (ret != 0)
    {
        if (y_cb->ymodem_abort)
        {
            y_cb->ymodem_abort(YMODEM_ERR_HEADER);
        }
        return YMODEM_ERR_HEADER;
    }

    if (y_cb->ymodem_start)
    {
        y_head.file_name = cache_frame.buf;
        y_head.name_len  = fname_len;
        y_head.file_size = cache_frame.file_size;
        y_cb->ymodem_start(&y_head);
    }

    y_port->ymodem_putchar(YMODEM_ACK);
    y_port->ymodem_putchar(YMODEM_C);

    y_data.offset = 0;

    while (cache_frame.file_size > 0)
    {
        if (ymodem_receive_packet(&cache_frame, MAX_RETRY) != 0)
        {
            if (y_cb->ymodem_abort)
            {
                y_cb->ymodem_abort(YMODEM_ERR_DATA);
            }
            return YMODEM_ERR_DATA;
        }

        y_data.data    = cache_frame.buf;
        y_data.size    = cache_frame.pkg_size;

        if (y_cb->ymodem_recv_data)
        {
            y_cb->ymodem_recv_data(&y_data);
        }

        y_data.offset += cache_frame.pkg_size;
        cache_frame.file_size -= cache_frame.pkg_size;

        y_port->ymodem_putchar(YMODEM_ACK);
    }

    if (ymodem_receive_packet(&cache_frame, MAX_RETRY) == YMODEM_EOT)
    {
        y_port->ymodem_putchar(YMODEM_NAK);
        if (ymodem_receive_packet(&cache_frame, MAX_RETRY) == YMODEM_EOT)
        {
            y_port->ymodem_putchar(YMODEM_ACK);
            y_port->ymodem_putchar(YMODEM_C);
            if (ymodem_receive_packet(&cache_frame, MAX_RETRY) == 0)
            {
                for (i = 0; i < cache_frame.pkg_size; i++)
                {
                    if (cache_frame.buf[i] != 0x00)
                    {
                        end_err = 1;
                        break;
                    }
                }

                if (end_err == 0)
                {
                    y_port->ymodem_putchar(YMODEM_ACK);
                    if (y_cb->ymodem_finish)
                    {
                        y_cb->ymodem_finish();
                        y_port->ymodem_putchar(RECV_END_CHAR);
                    }
                }
                else
                {
                    if (y_cb->ymodem_abort)
                    {
                        y_cb->ymodem_abort(YMODEM_ERR_EOT);
                    }
                    return YMODEM_ERR_EOT;
                }
            }
        }
    }

    return YMODEM_OK;
}
