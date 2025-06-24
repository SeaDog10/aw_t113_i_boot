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

static void (*ymodem_putchar)(void *, char)                 = YMODEM_NULL;
static int  (*ymodem_getchar)(void *, char *, unsigned int) = YMODEM_NULL;
static void *ymodem_putchar_arg                             = YMODEM_NULL;
static void *ymodem_getchar_arg                             = YMODEM_NULL;

static struct ymodem_frame cache_frame     = {0};

#ifdef SUPPORT_1K
static char   buffer_cache[PACKET_SIZE_1K]   = {0};
#else
static char   buffer_cache[PACKET_SIZE_128]   = {0};
#endif

static unsigned short ymodem_crc16(char *buf, unsigned short len)
{
    unsigned short crc = 0;

    for (unsigned short i = 0; i < len; ++i)
    {
        crc ^= ((unsigned short)buf[i] << 8);
        for (unsigned char j = 0; j < 8; ++j)
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

int ymodem_init(void (*putchar)(void *, char), void *putarg,
    int (*getchar)(void *, char *, unsigned int), void *getarg)
{
    if (putchar == YMODEM_NULL || getchar == YMODEM_NULL)
    {
        return -1;
    }

    ymodem_putchar     = putchar;
    ymodem_putchar_arg = putarg;
    ymodem_getchar     = getchar;
    ymodem_getchar_arg = getarg;

    return 0;
}

static int ymodem_receive_packet(struct ymodem_frame *frame, int retries)
{
    int size = 0;
    unsigned short recv_crc = 0;
    unsigned short calc_crc = 0;

    while (retries > 0)
    {
        if (ymodem_getchar(ymodem_getchar_arg, &frame->head, 10) != 0)
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

        if(ymodem_getchar(ymodem_getchar_arg, &frame->pack_num, 10) != 0)
        {
            retries--;
            ymodem_putchar(ymodem_putchar_arg, YMODEM_NAK);
            continue;
        }

        if(ymodem_getchar(ymodem_getchar_arg, &frame->pack_mask, 10) != 0)
        {
            retries--;
            ymodem_putchar(ymodem_putchar_arg, YMODEM_NAK);
            continue;
        }

        if ((frame->pack_num ^ frame->pack_mask) != 0xFF)
        {
            retries--;
            ymodem_putchar(ymodem_putchar_arg, YMODEM_NAK);
            continue;
        }

        frame->offset = 0;
        size = frame->pkg_size;

        while (size > 0)
        {
            if (ymodem_getchar(ymodem_getchar_arg, &frame->buf[frame->offset], 10) == 0)
            {
                frame->offset++;
                size--;
            }
        }

        if(ymodem_getchar(ymodem_getchar_arg, &frame->crc_h, 10) != 0)
        {
            retries--;
            ymodem_putchar(ymodem_putchar_arg, YMODEM_NAK);
            continue;
        }

        if(ymodem_getchar(ymodem_getchar_arg, &frame->crc_l, 10) != 0)
        {
            retries--;
            ymodem_putchar(ymodem_putchar_arg, YMODEM_NAK);
            continue;
        }

        recv_crc = ((frame->crc_h << 8) & 0xFF00) | (frame->crc_l & 0x00FF);
        calc_crc = ymodem_crc16(frame->buf, frame->pkg_size);

        if (recv_crc != calc_crc)
        {
            retries--;
            ymodem_putchar(ymodem_putchar_arg, YMODEM_NAK);
            continue;
        }

        return 0;
    }

    return -1;
}

int ymodem_receive(char *file_name, unsigned int name_len, char *file_data, unsigned int data_len)
{
    int begin_recv = 0;
    int i = 0;
    int ret = 0;
    int fname_len = 0;
    int fsize_len = 0;
    int recv_len = 0;
    int end_err = 0 ;

    cache_frame.buf = buffer_cache;

    for (i = 0; i < MAX_RETRY; i++)
    {
        ymodem_putchar(ymodem_putchar_arg, YMODEM_C);
        if (ymodem_receive_packet(&cache_frame, MAX_RETRY) == 0)
        {
            begin_recv = 1;
            break;
        }
    }

    if (begin_recv == 0)
    {
        return -1;
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
        return -1;
    }

    if (name_len >= (fname_len - 1))
    {
        for (i = 0; i < (fname_len - 1); i++)
        {
            file_name[i] = cache_frame.buf[i];
        }
    }
    else
    {
        for (i = 0; i < name_len; i++)
        {
            file_name[i] = cache_frame.buf[i];
        }
    }

    ymodem_putchar(ymodem_putchar_arg, YMODEM_ACK);
    ymodem_putchar(ymodem_putchar_arg, YMODEM_C);

    while (cache_frame.file_size > 0)
    {
        if (ymodem_receive_packet(&cache_frame, MAX_RETRY) != 0)
        {
            return -1;
        }

        if (data_len >= cache_frame.pkg_size)
        {
            for (i = 0; i < cache_frame.pkg_size; i++)
            {
                file_data[recv_len] = cache_frame.buf[i];
                recv_len++;
            }
            data_len -= cache_frame.pkg_size;
        }
        else if (data_len > 0)
        {
            for (i = 0; i < data_len; i++)
            {
                file_data[recv_len] = cache_frame.buf[i];
                recv_len++;
            }
            data_len = 0;
        }

        cache_frame.file_size -= cache_frame.pkg_size;
        ymodem_putchar(ymodem_putchar_arg, YMODEM_ACK);
    }

    if (ymodem_receive_packet(&cache_frame, MAX_RETRY) == YMODEM_EOT)
    {
        ymodem_putchar(ymodem_putchar_arg, YMODEM_NAK);
        if (ymodem_receive_packet(&cache_frame, MAX_RETRY) == YMODEM_EOT)
        {
            ymodem_putchar(ymodem_putchar_arg, YMODEM_ACK);
            ymodem_putchar(ymodem_putchar_arg, YMODEM_C);
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
                    ymodem_putchar(ymodem_putchar_arg, YMODEM_ACK);
                    ymodem_putchar(ymodem_putchar_arg, 0x4f);
                }
            }
        }
    }

    return recv_len;
}
