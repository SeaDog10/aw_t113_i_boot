#include "openamp_app.h"

static int echo_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    ((uint8_t *)data)[len] = 0;

    printf("cpu#%d echo_cb src:0x%X, len:%d, data:%s\n", 0, src, len, data);

    return 0;
}

static void echo_unbind_cb(struct rpmsg_endpoint *ept)
{
    printf("echo_unbind_cb ept addr = 0x%X  dest_addr = 0x%X name = %s\n", ept->addr, ept->dest_addr, ept->name);
    openamp_app_node_unregister(ept->name);
}

int app_echo_init(void)
{
    struct openamp_app_node *echo_node = NULL;

    echo_node = openamp_app_node_register(ECHO_NAME, echo_cb, echo_unbind_cb, openamp_dev_get());
    if(echo_node == NULL)
    {
        printf("echo_node is NULL\n");
        return -1;
    }

    return 0;
}
#ifdef USER_RT_THREAD_RTOS
MSH_CMD_EXPORT(app_echo_init, app echo init);
#endif

int app_echo_deinit(void)
{
    openamp_app_node_unregister(ECHO_NAME);
    return 0;
}
#ifdef USER_RT_THREAD_RTOS
MSH_CMD_EXPORT(app_echo_deinit, app echo deinit);
#endif

int app_echo_send(char *send_buf)
{
    int                      ret       = -1;
    struct openamp_app_node *echo_node = NULL;

    echo_node = openamp_find_app_node(ECHO_NAME);
    if(echo_node == NULL)
    {
        printf("echo_node is NULL\n");
        return ret;
    }

    if(openamp_app_send(&echo_node->ept, send_buf, strlen(send_buf)) == strlen(send_buf))
    {
        ret = 0;
    }

    return ret;
}

#ifdef USER_RT_THREAD_RTOS
int app_echo(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("app echo send cmd error\n");
        return -1;
    }

    app_echo_send(argv[1]);

    return 0;
}
MSH_CMD_EXPORT(app_echo, app echo data);
#endif
