#ifndef __YMODEM_H__
#define __YMODEM_H__

#define SUPPORT_1K

#define YMDOEM_MAX_RETRY    100

#define RECV_END_CHAR       0x4f

#define YMODEM_LOG_NONE     0
#define YMODEM_LOG_ERROR    1
#define YMODEM_LOG_WARN     2
#define YMODEM_LOG_INFO     3
#define YMODEM_LOG_DEBUG    4
#define YMODEM_LOG_TRACE    5

#define YMODEM_LOG_LEVEL    YMODEM_LOG_ERROR

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
}ymodem_errcode_t;

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
    void (*ymodem_abort)(ymodem_errcode_t err);
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
