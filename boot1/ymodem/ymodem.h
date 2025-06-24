#ifndef __YMODEM_H__
#define __YMODEM_H__

#define MAX_RETRY       100
#define SUPPORT_1K

int ymodem_init(void (*putchar)(void *, char), void *putarg,
    int (*getchar)(void *, char *, unsigned int), void *getarg);

int ymodem_receive(char *file_name, unsigned int name_len,
    char *file_data, unsigned int data_len);

#endif
