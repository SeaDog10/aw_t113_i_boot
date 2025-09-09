#ifndef __OPENAMP_APP_H__
#define __OPENAMP_APP_H__

#include "amp_sigiface_dev.h"
#include "platform_port.h"
#include <openamp_virtio_device.h>

#define USER_RT_THREAD_RTOS
#define ECHO_NAME "echo"

int app_echo_init(void);
int app_echo_send(char *send_buf);

#endif /* __OPENAMP_APP_H__ */