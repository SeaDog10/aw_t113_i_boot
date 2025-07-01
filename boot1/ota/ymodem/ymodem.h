#ifndef __YMODEM_H__
#define __YMODEM_H__

#define MAX_RETRY       100

#define SUPPORT_1K

#define RECV_END_CHAR   0x4f

typedef enum
{
    YMODEM_OK          =  0,
    YMODEM_ERR_PARAM   = -1,
    YMODEM_ERR_TIMEOUT = -2,
    YMODEM_ERR_HEADER  = -3,
    YMODEM_ERR_SIZE    = -4,
    YMODEM_ERR_DATA    = -5,
    YMODEM_ERR_CRC     = -6,
    YMODEM_ERR_EOT     = -7,
    YMODEM_ERR_ABORTED = -8,
    YMODEM_ERR_UNKNOWN = -9,
}ymodem_errcode;

typedef struct ymodem_head
{
    const char   *file_name;
    unsigned int  name_len;
    unsigned int  file_size;
} ymodem_head_t;

typedef struct ymodem_data
{
    const char   *data;
    unsigned int offset;
    unsigned int size;
} ymodem_data_t;

typedef struct ymodem_callback
{
    void (*ymodem_start)(ymodem_head_t *head);
    void (*ymodem_recv_data)(ymodem_data_t *data);
    void (*ymodem_abort)(ymodem_errcode err);
    void (*ymodem_finish)(void);
} ymodem_callback_t;

typedef struct ymdoem_port
{
    void (*ymodem_putchar)(char ch);
    int  (*ymodem_getchar)(char *ch, unsigned int timeout);
} ymdoem_port_t;

int ymodem_init(ymdoem_port_t *port);
int ymodem_receive(ymodem_callback_t *callback);

#endif
