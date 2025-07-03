#ifndef __YMODEM_H__
#define __YMODEM_H__

#define SUPPORT_1K                  /* Enable support for 1K YMODEM packets */

#define YMDOEM_MAX_RETRY    100     /* Maximum number of retry attempts for YMODEM operations */

#define RECV_END_CHAR       0x4f    /* End character for YMODEM receive operation */

#define YMODEM_LOG_NONE     0       /* YMODEM log level: no output */
#define YMODEM_LOG_ERROR    1       /* YMODEM log level: error output */
#define YMODEM_LOG_WARN     2       /* YMODEM log level: warning output */
#define YMODEM_LOG_INFO     3       /* YMODEM log level: informational output */
#define YMODEM_LOG_DEBUG    4       /* YMODEM log level: debug output */
#define YMODEM_LOG_TRACE    5       /* YMODEM log level: trace output */

#define YMODEM_LOG_LEVEL    YMODEM_LOG_ERROR    /* Set the current YMODEM log level */

/* ymodem_errcode_t: Error codes for YMODEM operations. */
typedef enum
{
    YMODEM_OK          =  0,   /* Operation successful */
    YMODEM_ERR_PARAM   = -1,   /* Invalid parameter */
    YMODEM_ERR_TIMEOUT = -2,   /* Timeout occurred */
    YMODEM_ERR_HEADER  = -3,   /* Header error */
    YMODEM_ERR_SIZE    = -4,   /* Size error */
    YMODEM_ERR_DATA    = -5,   /* Data error */
    YMODEM_ERR_CRC     = -6,   /* CRC check failed */
    YMODEM_ERR_EOT     = -7,   /* End of transmission error */
    YMODEM_ERR_ABORTED = -8,   /* Transmission aborted */
    YMODEM_ERR_UNKNOWN = -9,   /* Unknown error */
} ymodem_errcode_t; /* ymodem_errcode_t: Error codes for YMODEM operations. */

/* ymodem_head_t: YMODEM file header information. */
typedef struct ymodem_head
{
    const char   *file_name;   /* Name of the file being transferred */
    unsigned int  name_len;    /* Length of the file name */
    unsigned int  file_size;   /* Size of the file in bytes */
} ymodem_head_t; /* ymodem_head_t: YMODEM file header information. */

/* ymodem_data_t: YMODEM data packet information. */
typedef struct ymodem_data
{
    const char   *data;        /* Pointer to data buffer */
    unsigned int offset;       /* Offset in the file */
    unsigned int size;         /* Size of the data in bytes */
} ymodem_data_t; /* ymodem_data_t: YMODEM data packet information. */

/* ymodem_callback_t: Callback functions for YMODEM transfer events. */
typedef struct ymodem_callback
{
    void (*ymodem_start)(ymodem_head_t *head);         /* Called when YMODEM transfer starts */
    void (*ymodem_recv_data)(ymodem_data_t *data);     /* Called when data is received */
    void (*ymodem_abort)(ymodem_errcode_t err);        /* Called when transfer is aborted */
    void (*ymodem_finish)(void);                       /* Called when transfer finishes */
} ymodem_callback_t; /* ymodem_callback_t: Callback functions for YMODEM transfer events. */

/* ymdoem_port_t: YMODEM port interface for low-level input/output. */
typedef struct ymdoem_port
{
    void (*ymodem_putchar)(char ch);                       /* Output a character */
    int  (*ymodem_getchar)(char *ch, unsigned int timeout);/* Input a character with timeout */
} ymdoem_port_t; /* ymdoem_port_t: YMODEM port interface for low-level input/output. */

int ymodem_init(ymdoem_port_t *port);
int ymodem_receive(ymodem_callback_t *callback);

#endif /* __YMODEM_H__ */
